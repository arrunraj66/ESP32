# Server Guide: set up the free server first (no ESP32 needed)

Goal: get the micro-ZMQ broker and agent running on a free cloud server, and prove they work
with a **simulated robot**. When the real ESP32 arrives, you only change the IP address in `config.h`.

## 1. What goes on the server
| Phase | On the server | Status |
|---|---|---|
| 1 (now) | `microzmq_broker` + `microzmq_agent` (both run as services) | ready |
| 2 | recorder: log `robot/out/*` into SQLite (reuse your AUV recorder) | later |
| 3 | dashboard / remote control page | later |
| not here | speech-to-text, LLM, text-to-speech | run on your PC or use an API; free servers are too small |

The broker only listens on `127.0.0.1`. Only the **agent port 7400** is reachable from outside.

## 2. Choose a free server
Free offers change often, so confirm current limits on the provider's own page before you sign up.
- **Oracle Cloud Always Free:** reports in 2026 say the free ARM allowance was reduced to 2 OCPU / 12 GB RAM,
  plus two tiny AMD VMs (1 GB each). That is far more than this project needs. Possible problems:
  "out of capacity" errors when creating the VM, and idle accounts can be reclaimed. Your home region
  cannot be changed later, so pick a nearby one if offered (for example Mumbai or Hyderabad).
- **Google Cloud e2-micro:** reported as permanently free with 1 GB RAM, which is enough for the broker and agent.
  Check which regions are included in the free offer.
Either one works. Choose **Ubuntu 22.04 or 24.04** as the image.

## 3. Create the VM
1. Create an account and a VM with Ubuntu. Download or add your SSH key.
2. In the provider's **firewall / security list / VPC rules**, allow inbound **TCP 7400** (and TCP 22 for SSH).
   Do not open 5555 or 5556.
3. Note the VM's **public IP address**.

## 4. Install on the server
```
ssh ubuntu@SERVER_IP            # user may be 'ubuntu' or 'opc' depending on provider
sudo apt update && sudo apt install -y git
git clone https://github.com/arrunraj66/ESP32.git
cd ESP32
sudo bash deploy/setup_server.sh
```
The script builds the C++ tools, creates a random token, and starts both services (they restart on boot
and after crashes). It prints the token at the end. **Copy the token.**

Check the services:
```
systemctl status microzmq-broker microzmq-agent
journalctl -u microzmq-agent -f        # live agent log
```
On Oracle Ubuntu images you may also need the `iptables` lines the script prints.

## 5. Test from your own PC (simulated robot)
```
python3 tests/fake_esp.py --host SERVER_IP --token YOUR_TOKEN
```
Expected: `ACK received: connected and authenticated`. The server log shows `node 'fake-robot' connected`.

Now send a command **from the server** (second SSH session):
```
microzmq_tool pub robot/in/emotion happy
```
The simulated robot prints `robot received: robot/in/emotion happy`.

Watch what the "robot" publishes (distance every second):
```
microzmq_tool sub robot/out/
```
If this all works, the server side is finished.

## 6. When the ESP32 arrives
In `firmware/robot_firmware/config.h` set:
```
#define ZMQ_AGENT_HOST  "SERVER_IP"
#define ZMQ_TOKEN       "YOUR_TOKEN"
```
plus your WiFi name and password. Nothing on the server changes.

## 7. Security (important)
- The token is a shared password sent in **plain text**. Anyone watching the network could read it.
  Use a long random token (the script makes one), never reuse a real password, and treat this as a
  hobby setup until encryption is added.
- Optional hardening now: in the cloud firewall, allow port 7400 only from your home IP, if it is stable.
- Never open ports 5555/5556. Keep the VM updated (`sudo apt update && sudo apt upgrade`).
- Next step I recommend: TLS on port 7400 (a TLS wrapper on the server plus a secure client on the ESP32).

## 8. Notes
- A cloud server adds network delay (tens of milliseconds). That is fine for commands and status.
  Keep fast reactions (blinking, idle motion, distance reaction) on the ESP32, as the firmware already does.
- To update later: `cd ESP32 && git pull && sudo bash deploy/setup_server.sh` (the token is kept).
- If the agent port is blocked by your home network or ISP, test from a phone hotspot to find out.
