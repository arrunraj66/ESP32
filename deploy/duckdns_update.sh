#!/usr/bin/env bash
# Tells DuckDNS this server's current public IP. Reads DUCKDNS_DOMAIN and DUCKDNS_TOKEN
# from /etc/microzmq.env. Run by microzmq-duckdns.timer every 5 minutes.
set -euo pipefail
source /etc/microzmq.env
: "${DUCKDNS_DOMAIN:?not set}" "${DUCKDNS_TOKEN:?not set}"
# leaving ip= empty makes DuckDNS use the address the request came from
curl -fsS --max-time 20 "https://www.duckdns.org/update?domains=${DUCKDNS_DOMAIN}&token=${DUCKDNS_TOKEN}&ip=" -o /dev/null
