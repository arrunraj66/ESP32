# micro-ZMQ  (ESP32 <-> ZeroMQ), like micro-ROS but for ZeroMQ

```
ESP32 (microzmq client) --TCP--> agent.py --ZeroMQ PUB/SUB--> broker.py (XSUB/XPUB) <--> your nodes
```
The ESP32 never runs ZeroMQ. It speaks a tiny framed TCP protocol to an agent that
republishes everything on your real ZeroMQ network (same idea as the micro-ROS agent).

## Frame format (little endian)
    u16 length      number of bytes after this field (type + body)
    u8  type
    ... body

| type | name  | direction      | body                                   |
|------|-------|----------------|----------------------------------------|
| 1    | HELLO | ESP -> agent   | token, 0x00, node name                 |
| 6    | ACK   | agent -> ESP   | empty (HELLO accepted)                 |
| 2    | SUB   | ESP -> agent   | topic prefix (raw bytes)               |
| 3    | PUB   | both           | u8 topic_len, topic, payload           |
| 4    | PING  | ESP -> agent   | empty                                  |
| 5    | PONG  | agent -> ESP   | empty                                  |

Max frame: 1024 bytes on the ESP32 (MZ_MAX_FRAME), 8192 in the agent.
Payload is opaque bytes (text, JSON or protobuf). The agent does not look inside it.

## ZeroMQ side
Messages are 2 frames: [topic][payload]. Topic prefix matching is standard ZeroMQ.

## Topic naming used by the robot firmware
    robot/out/<topic>   robot -> network   (robot/out/distance, robot/out/audio/loud)
    robot/in/<topic>    network -> robot   (robot/in/emotion, robot/in/servo/cmd)
The ESP32 subscribes to "robot/in/" and publishes under "robot/out/", so there are no loops.

## Security
- Token in HELLO is a shared secret, NOT encryption. On a home LAN it is fine.
- Never expose the broker ports (5555/5556) to the internet. Broker binds 127.0.0.1 by default.
- Over the internet put the agent port behind TLS (stunnel / reverse proxy) or a VPN.
