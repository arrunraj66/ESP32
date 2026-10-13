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
| `cpp/` | micro-ZMQ agent, broker and test tool in **C++** (libzmq, CMake). |
| `agent/` | The same three tools in Python (optional alternative). |
| `tests/audio_test/` | Stand-alone mic + speaker bench test. |
| `tests/fake_esp.py` | Simulated robot to test the server before the board arrives. |
| `docs/USER_MANUAL.md` | **Full build and usage manual. Start here.** |
| `docs/SERVER_GUIDE.md` | Set up the free cloud server first, no ESP32 needed. |
| `deploy/` | One-shot server install script and systemd services. |
| `docs/PROTOCOL.md` | micro-ZMQ wire protocol. |

## Quick start
1. Read `docs/USER_MANUAL.md` (power and wiring first, especially for the servos).
2. Flash `tests/audio_test` to check the mic and speaker.
3. Flash `firmware/robot_firmware`, one module at a time (switches in `config.h`).
4. On your PC build and run the C++ tools:
   ```
   sudo apt install -y build-essential cmake pkg-config libzmq3-dev
   cmake -S cpp -B cpp/build && cmake --build cpp/build -j"$(nproc)"
   ./cpp/build/microzmq_broker &
   ./cpp/build/microzmq_agent --token change-me
   ```

## Status
- C++ and Python agent/broker: both tested with a simulated ESP32 client (handshake, bad-token rejection, both message directions).
- ESP32 firmware: written but **not yet compiled or run on hardware**. Expect small fixes on first build.
