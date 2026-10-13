# Desktop Robot: User Manual

## 1. What you are building
A desktop robot with:
- animated eyes on a 2.8" ILI9341 TFT (neutral, happy, sad, angry, blinking, looking around)
- 5 servos (head pan, head tilt, left arm, right arm, base)
- INMP441 I2S microphone and MAX98357A I2S amplifier with a small speaker
- VL53L1X distance sensor (reacts when something comes close)
- a WiFi link (micro-ZMQ) to a PC running ZeroMQ, so a "brain" on the PC can control it

The firmware is split into **modules** that talk through a small message bus, so parts can be
enabled, disabled or replaced without touching the rest.

## 2. Parts list
| Part | Notes |
|---|---|
| ESP32-S3 dev board | normal (non-camera) board is fine |
| Mini ESP32-S3 (optional) | later, to move audio onto a second board if servos jitter |
| 5x SG90 / 9g servos | |
| INMP441 I2S mic | |
| MAX98357A I2S amplifier + small speaker (4-8 ohm, 3 W) | |
| 2.8" ILI9341 SPI TFT | |
| VL53L1X breakout (I2C) | |
| 8x 18650 cells, **2S** BMS, 5 V 5 A buck converter | servo power (see section 3) |
| Switch, 3-5 A fuse, 1000 uF capacitor, thick wire | safety and power |
| Optional: ESP32-S3-CAM or USB webcam | vision, later |

The 4S BMS is **not** used here (14.8 V is too high for 9g servos).

## 3. Power system (read before connecting servos)
**This is the part most likely to go wrong. 18650 cells can cause fires if handled wrongly.**

- Build a **2S pack (7.4 V nominal, 8.4 V full)** with a 2S BMS. Use the 8 cells as 2S4P, or use fewer.
  Never mix old and new cells, and never charge without a proper 2S BMS or balance charger.
- Put a **switch and 3-5 A fuse** in the battery positive line.
- A **5 V 5 A buck converter** feeds the servos. Set it to 5.0-5.5 V with a multimeter **before** connecting anything.
- Add a **1000 uF capacitor** across the servo 5 V rail, close to the servos.
- **Never power servos from the ESP32 5 V pin.** Use thick wires for servo power.
- **Join all grounds together** (battery, buck, ESP32, servo, amp).
- Power the ESP32 from the same buck output (its 5 V pin) or USB, but keep servo wiring separate.
- Keep cells away from moving parts and heat. Use holders or properly spot-welded strips.

## 4. Wiring (default pins from `config.h`)
Avoid GPIO 26-37 on boards with PSRAM or octal flash.

**Servos** (signal wires; power from the buck, not the ESP32)
| Servo | GPIO |
|---|---|
| Head pan | 1 |
| Head tilt | 2 |
| Left arm | 42 |
| Right arm | 41 |
| Base | 40 |

**TFT ILI9341 (SPI)**
| TFT | ESP32-S3 |
|---|---|
| SCK | 12 |
| MOSI (SDI) | 11 |
| MISO (SDO) | 13 |
| CS | 10 |
| DC | 14 |
| RST | 21 |
| VCC, LED | 3.3 V |
| GND | GND |

**INMP441 mic**
| INMP441 | ESP32-S3 |
|---|---|
| VDD | 3.3 V |
| GND | GND |
| SCK | 4 |
| WS | 5 |
| SD | 6 |
| L/R | GND |

**MAX98357A amp**
| MAX98357A | ESP32-S3 |
|---|---|
| VIN | 5 V |
| GND | GND |
| BCLK | 7 |
| LRC | 15 |
| DIN | 16 |
| SD, GAIN | not connected |
| Speaker | to the amp's output terminals only |

**VL53L1X (I2C)**
| VL53L1X | ESP32-S3 |
|---|---|
| VIN | 3.3 V |
| GND | GND |
| SDA | 8 |
| SCL | 9 |

## 5. Software setup
1. Install **Arduino IDE 2.x**.
2. Boards Manager: install **esp32 by Espressif Systems, version 2.0.17** (the audio code uses the legacy I2S driver).
3. Select board **ESP32S3 Dev Module**, set **USB CDC On Boot: Enabled**.
4. Library Manager, install: **ESP32Servo**, **Adafruit GFX Library**, **Adafruit ILI9341**, **VL53L1X** (by Pololu).
5. Open `firmware/robot_firmware/robot_firmware.ino`. All files in that folder are part of the sketch.

## 6. Bring-up: test one thing at a time
Turn modules on one at a time with the switches at the top of `config.h`
(`USE_SERVOS`, `USE_DISPLAY`, `USE_AUDIO`, `USE_DISTANCE`, `USE_BEHAVIOR`, `USE_ZMQ`, `USE_COMMS`).
Keep `USE_COMMS 1` so you can send test commands.

| Step | Switch on | What to check |
|---|---|---|
| 1 | `tests/audio_test` sketch | 3 beeps; Serial Monitor bars jump when you clap |
| 2 | `USE_DISPLAY` | eyes appear and blink; try `emotion happy` |
| 3 | `USE_DISTANCE` | `distance` values print when you move a hand in front |
| 4 | `USE_AUDIO` | `audio/beep 880,200` beeps |
| 5 | `USE_SERVOS` (power system built and checked first) | `servo/cmd 0,60,60` moves the head slowly |
| 6 | `USE_BEHAVIOR` | idle look-around; happy face and beep when you come within 40 cm |
| 7 | `USE_ZMQ` | see section 8 |

If a module prints `FAILED (skipped)` at boot, disable the others and test it alone.

## 7. Serial commands
Open the Serial Monitor at **115200 baud**, set line ending to Newline, and type `topic payload`:

| Topic | Payload | Effect |
|---|---|---|
| `emotion` | `neutral`, `happy`, `sad`, `angry` | changes the eyes |
| `face/look` | `x,y` from -1.0 to 1.0 | eyes look in a direction |
| `servo/cmd` | `id,angle,speed` (e.g. `0,45,60`) | move servo `id` to `angle` at `speed` deg/s |
| `servo/center` | (any) | all servos to center |
| `audio/beep` | `freq,ms` (e.g. `880,200`) | play a tone |

Servo ids: 0 pan, 1 tilt, 2 left arm, 3 right arm, 4 base. Angle limits are set in `config.h`
(`SERVO_MIN`, `SERVO_MAX`); commands are clamped to them.

Topics the robot publishes: `distance` (mm), `audio/level`, `audio/loud`.
Which ones print on Serial is set by `OUTBOUND_TOPICS` in `config.h`.

## 8. Connecting to ZeroMQ (micro-ZMQ)
The ESP32 cannot run ZeroMQ, so it connects to an **agent** on a PC. The agent republishes everything on ZeroMQ.

On the PC, build the C++ tools (Ubuntu 22.04):
```
sudo apt install -y build-essential cmake pkg-config libzmq3-dev
cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpp/build -j"$(nproc)"
```
Then run each in its own terminal:
```
./cpp/build/microzmq_broker                    # ZeroMQ XSUB/XPUB broker (ports 5555/5556)
./cpp/build/microzmq_agent --token change-me   # listens for the ESP32 on port 7400
```
(Python versions of the same tools are in `agent/`: `python3 agent/broker.py` and
`python3 agent/agent.py --token change-me`. They behave the same; use either one.)

If your AUV middleware already has a broker, skip `broker.py` and run the agent with
`--xsub tcp://HOST:PORT --xpub tcp://HOST:PORT`.

In `firmware/robot_firmware/config.h` set `WIFI_SSID`, `WIFI_PASS`, `ZMQ_AGENT_HOST` (the PC's IP address),
and `ZMQ_TOKEN` (same as `--token`). Flash it. The Serial Monitor should show
`[zmq] WiFi connected` then `[zmq] agent connected`.

Test from the PC:
```
./cpp/build/microzmq_tool sub robot/out/                  # watch what the robot sends
./cpp/build/microzmq_tool pub robot/in/emotion happy      # change the robot's face
./cpp/build/microzmq_tool pub robot/in/servo/cmd 0,45,60  # move the head
```
Topic rule: the robot publishes `robot/out/<topic>` and listens on `robot/in/<topic>`.
Message bodies are plain bytes, so JSON or protobuf also work. See `docs/PROTOCOL.md`.

**Security:** the token is a shared secret, not encryption. Do not expose broker ports 5555/5556 to the internet.
For internet access, put the agent port behind TLS or a VPN.

## 9. Adding your own module
1. Copy any `*_module.h` / `*_module.cpp` pair and rename the class.
2. In `begin()`, set up hardware and call `Bus::subscribe("topic", handler)`.
3. In `update()`, do **non-blocking** work (no long `delay()`), and publish results with `Bus::publish("topic", payload, name())`.
4. In `robot_firmware.ino` add one `#include` and one `addModule(new YourModule());`.
5. Add a `USE_YOURMODULE` switch in `config.h`.

## 10. Troubleshooting
| Problem | Check |
|---|---|
| No beep | amp VIN on 5 V, shared ground, BCLK/LRC/DIN not swapped, speaker on amp output terminals |
| Mic reads 0 or constant max | L/R pin on GND, SCK/WS/SD pins, 3.3 V stable |
| Servos jitter or ESP32 resets | servos on their own buck supply, 1000 uF capacitor, common ground, thicker wires |
| Blank TFT | wiring, LED pin to 3.3 V, correct pins in `config.h` |
| Distance module FAILED | SDA/SCL pins, 3.3 V, I2C address |
| `[zmq]` never says agent connected | same WiFi network, PC firewall allows port 7400 (`sudo ufw allow 7400/tcp`), token matches, correct PC IP |
| Agent log says `rejected ... bad token` | `ZMQ_TOKEN` in `config.h` must equal the agent `--token` |
| Compile error about I2S | use esp32 core 2.0.17, not 3.x |
| Brief stutter every few seconds | agent is offline; the ESP32 retries every 3 s (up to 0.5 s block) |

## 11. Safety
- Use a fuse and a proper 2S BMS. Never leave charging 18650 cells unattended.
- Keep fingers clear of the arms and base when powering on.
- Test servo movement without the mechanism attached first, and set safe limits in `config.h`.

## 12. Roadmap
- Stream mic audio to the PC (`robot/out/audio/pcm`), Whisper speech-to-text, LLM, text-to-speech back to the speaker.
- Camera node (ESP32-S3-CAM or USB webcam) for face tracking.
- Protobuf messages in the AUV middleware `proto/` folder, dashboards, and SQLite recording of `robot/out/`.
- Move audio to the mini ESP32-S3 if servo jitter appears.
