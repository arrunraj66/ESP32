#!/usr/bin/env python3
"""micro-ZMQ agent: bridges ESP32 TCP clients to ZeroMQ PUB/SUB.  pip install pyzmq"""
import argparse, asyncio, logging, os, struct
import zmq, zmq.asyncio

T_HELLO, T_SUB, T_PUB, T_PING, T_PONG, T_ACK = 1, 2, 3, 4, 5, 6
MAX_FRAME = 8192
log = logging.getLogger("agent")


async def read_frame(reader):
    n = struct.unpack("<H", await reader.readexactly(2))[0]
    if n < 1 or n > MAX_FRAME:
        raise ValueError(f"bad frame length {n}")
    body = await reader.readexactly(n)
    return body[0], body[1:]


def send_frame(writer, ftype, body=b""):
    writer.write(struct.pack("<H", len(body) + 1) + bytes([ftype]) + body)


async def handle_client(reader, writer, args, ctx):
    peer = writer.get_extra_info("peername")
    pub = sub = fwd = None
    name = "?"
    try:
        ftype, body = await asyncio.wait_for(read_frame(reader), 5)
        if ftype != T_HELLO:
            raise ValueError("expected HELLO first")
        token, _, node = body.partition(b"\0")
        if args.token and token.decode(errors="replace") != args.token:
            log.warning("rejected %s: bad token", peer)
            return
        name = node.decode(errors="replace")

        pub = ctx.socket(zmq.PUB); pub.connect(args.xsub)
        sub = ctx.socket(zmq.SUB); sub.connect(args.xpub)
        await asyncio.sleep(0.1)                 # let ZeroMQ connect (slow joiner)
        send_frame(writer, T_ACK); await writer.drain()
        log.info("node '%s' connected from %s", name, peer)

        async def forward():                     # ZeroMQ -> ESP32
            while True:
                parts = await sub.recv_multipart()
                if len(parts) != 2:
                    continue
                topic, payload = parts
                if len(topic) > 255 or len(topic) + len(payload) + 2 > MAX_FRAME:
                    continue
                send_frame(writer, T_PUB, bytes([len(topic)]) + topic + payload)
                await writer.drain()

        fwd = asyncio.create_task(forward())

        while True:                              # ESP32 -> ZeroMQ
            ftype, body = await asyncio.wait_for(read_frame(reader), args.timeout)
            if ftype == T_SUB:
                sub.setsockopt(zmq.SUBSCRIBE, body)
                log.info("[%s] subscribe %r", name, body)
            elif ftype == T_PUB and body:
                tl = body[0]
                await pub.send_multipart([body[1:1 + tl], body[1 + tl:]])
            elif ftype == T_PING:
                send_frame(writer, T_PONG); await writer.drain()
    except (asyncio.IncompleteReadError, asyncio.TimeoutError, ConnectionError, ValueError) as e:
        log.info("node '%s' disconnected (%s)", name, type(e).__name__)
    finally:
        if fwd: fwd.cancel()
        for s in (pub, sub):
            if s: s.close(linger=0)
        writer.close()


async def main():
    p = argparse.ArgumentParser()
    p.add_argument("--listen", default="0.0.0.0")
    p.add_argument("--port", type=int, default=7400)
    p.add_argument("--xsub", default="tcp://127.0.0.1:5555", help="broker XSUB (publish here)")
    p.add_argument("--xpub", default="tcp://127.0.0.1:5556", help="broker XPUB (subscribe here)")
    p.add_argument("--token", default=os.environ.get("MICROZMQ_TOKEN", "change-me"))
    p.add_argument("--timeout", type=float, default=15, help="drop client after this many silent seconds")
    args = p.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")

    ctx = zmq.asyncio.Context()
    server = await asyncio.start_server(lambda r, w: handle_client(r, w, args, ctx), args.listen, args.port)
    log.info("agent listening on %s:%d  -> ZeroMQ %s / %s", args.listen, args.port, args.xsub, args.xpub)
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())
