#!/usr/bin/env python3
"""Simulated ESP32 robot, so you can test the server before the real board arrives.
It connects to the agent, subscribes to robot/in/, publishes a fake distance every second,
and prints every command it receives (emotion, servo/cmd, ...).

  python3 tests/fake_esp.py --host SERVER_IP --token YOUR_TOKEN
"""
import argparse, random, select, socket, struct, time

T_HELLO, T_SUB, T_PUB, T_PING, T_PONG, T_ACK = 1, 2, 3, 4, 5, 6


def frame(t, body=b""):
    return struct.pack("<H", len(body) + 1) + bytes([t]) + body


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7400)
    ap.add_argument("--token", default="change-me")
    ap.add_argument("--name", default="fake-robot")
    a = ap.parse_args()

    s = socket.create_connection((a.host, a.port), timeout=5)
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    s.sendall(frame(T_HELLO, a.token.encode() + b"\0" + a.name.encode()))
    s.sendall(frame(T_SUB, b"robot/in/"))
    print(f"connected to {a.host}:{a.port}, waiting for ACK ...")

    rx = b""
    ready = False
    last_pub = last_ping = time.time()
    while True:
        r, _, _ = select.select([s], [], [], 0.2)
        if r:
            d = s.recv(4096)
            if not d:
                print("server closed the connection (wrong token?)")
                return
            rx += d
            while len(rx) >= 2:
                n = struct.unpack("<H", rx[:2])[0]
                if len(rx) < 2 + n:
                    break
                t, body, rx = rx[2], rx[3:2 + n], rx[2 + n:]
                if t == T_ACK:
                    ready = True
                    print("ACK received: connected and authenticated")
                elif t == T_PUB:
                    tl = body[0]
                    print("robot received:", body[1:1 + tl].decode(errors="replace"),
                          body[1 + tl:].decode(errors="replace"))
        now = time.time()
        if ready and now - last_pub >= 1:
            last_pub = now
            topic = b"robot/out/distance"
            payload = str(random.randint(100, 800)).encode()
            s.sendall(frame(T_PUB, bytes([len(topic)]) + topic + payload))
        if now - last_ping >= 5:
            last_ping = now
            s.sendall(frame(T_PING))


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
