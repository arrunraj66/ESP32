#!/usr/bin/env bash
# Usage on the server:  sudo bash deploy/setup_duckdns.sh   (it asks for your DuckDNS name and token)
set -euo pipefail
if [ "$EUID" -ne 0 ]; then echo "Please run with sudo."; exit 1; fi
REPO="$(cd "$(dirname "$0")/.." && pwd)"
read -rp "DuckDNS subdomain (only the part before .duckdns.org): " DOM
read -rsp "DuckDNS token (typing is hidden): " TOK; echo
sed -i '/^DUCKDNS_/d' /etc/microzmq.env
printf 'DUCKDNS_DOMAIN=%s\nDUCKDNS_TOKEN=%s\n' "$DOM" "$TOK" >> /etc/microzmq.env
install -m 755 "$REPO/deploy/duckdns_update.sh" /usr/local/bin/duckdns_update.sh
install -m 644 "$REPO/deploy/microzmq-duckdns.service" "$REPO/deploy/microzmq-duckdns.timer" /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now microzmq-duckdns.timer
systemctl start microzmq-duckdns.service && echo "Updated. Check https://www.duckdns.org that ${DOM}.duckdns.org now shows this server's IP."
