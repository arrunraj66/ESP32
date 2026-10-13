# ESP32 Desktop Robot

A desktop robot built on an **ESP32-S3**: animated eyes on a 2.8" TFT, 5 servos, a mic and speaker,
a distance sensor, and a **micro-ZMQ** link that connects it to a ZeroMQ network
(same idea as micro-ROS, but for ZeroMQ).

```
ESP32-S3 robot --WiFi/TCP--> agent.py --ZeroMQ--> broker.py <--> your PC nodes / AUV middleware
```

| Folder | What is inside |
|---|---|
| `firmware/robot_firmware/` | Modular ESP32-S3 firmware (Arduino). Open `robot_firmware.ino`. |
| `agent/` | micro-ZMQ agent, ZeroMQ broker, and a test tool (Python). |
| `tests/audio_test/` | Stand-alone mic + speaker bench test. |
| `docs/USER_MANUAL.md` | **Full build and usage manual. Start here.** |
| `docs/PROTOCOL.md` | micro-ZMQ wire protocol. |

## Quick start
1. Read `docs/USER_MANUAL.md` (power and wiring first, especially for the servos).
2. Flash `tests/audio_test` to check the mic and speaker.
3. Flash `firmware/robot_firmware`, one module at a time (switches in `config.h`).
4. On your PC: `pip install -r agent/requirements.txt`, then run `agent/broker.py` and `agent/agent.py`.

## Status
- Agent and broker: tested with a simulated ESP32 client.
- ESP32 firmware: written but **not yet compiled or run on hardware**. Expect small fixes on first build.
