#!/usr/bin/env bash
# One-shot setup for a fresh Ubuntu 22.04/24.04 server (x86 or ARM).
# Run from the repo folder:   sudo bash deploy/setup_server.sh
set -euo pipefail

if [ "$EUID" -ne 0 ]; then echo "Please run with sudo."; exit 1; fi
REPO="$(cd "$(dirname "$0")/.." && pwd)"

echo "==> Installing build tools and libzmq"
apt-get update
apt-get install -y build-essential cmake pkg-config libzmq3-dev openssl python3-zmq

echo "==> Building the C++ tools"
cmake -S "$REPO/cpp" -B "$REPO/cpp/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$REPO/cpp/build" -j"$(nproc)"
install -m 755 "$REPO/cpp/build/microzmq_broker" "$REPO/cpp/build/microzmq_agent" \
               "$REPO/cpp/build/microzmq_tool" /usr/local/bin/

echo "==> Creating service user and token"
id microzmq >/dev/null 2>&1 || useradd --system --no-create-home --shell /usr/sbin/nologin microzmq
if [ ! -f /etc/microzmq.env ]; then
  echo "MICROZMQ_TOKEN=$(openssl rand -hex 16)" > /etc/microzmq.env
  chown root:microzmq /etc/microzmq.env
  chmod 640 /etc/microzmq.env
fi
if ! grep -q MICROZMQ_WEB_PASS /etc/microzmq.env; then
  echo "MICROZMQ_WEB_PASS=$(openssl rand -hex 6)" >> /etc/microzmq.env
fi
mkdir -p /opt/microzmq
install -m 644 "$REPO/agent/web_control.py" /opt/microzmq/web_control.py

echo "==> Installing and starting services"
install -m 644 "$REPO/deploy/microzmq-broker.service" /etc/systemd/system/
install -m 644 "$REPO/deploy/microzmq-agent.service"  /etc/systemd/system/
install -m 644 "$REPO/deploy/microzmq-web.service"    /etc/systemd/system/
systemctl daemon-reload
systemctl enable microzmq-broker microzmq-agent microzmq-web
systemctl restart microzmq-broker microzmq-agent microzmq-web
sleep 1
systemctl --no-pager --lines=3 status microzmq-broker microzmq-agent microzmq-web || true

echo
echo "=============================================================="
echo " Done. Your token (put it in config.h as ZMQ_TOKEN):"
grep MICROZMQ_TOKEN /etc/microzmq.env
echo " Phone page password (open http://SERVER_IP:8080):"
grep MICROZMQ_WEB_PASS /etc/microzmq.env
echo
echo " NEXT: allow TCP ports 7400 and 8080 in your cloud provider's firewall."
echo "   Oracle Ubuntu images may also need:"
echo "     sudo iptables -I INPUT 6 -m state --state NEW -p tcp --dport 7400 -j ACCEPT"
echo "     sudo iptables -I INPUT 6 -m state --state NEW -p tcp --dport 8080 -j ACCEPT"
echo "     sudo netfilter-persistent save"
echo " Do NOT open ports 5555/5556 (the broker listens on localhost only)."
echo "=============================================================="
