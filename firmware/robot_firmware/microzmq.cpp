#include "microzmq.h"

enum { T_HELLO = 1, T_SUB = 2, T_PUB = 3, T_PING = 4, T_PONG = 5, T_ACK = 6 };

void MicroZmq::begin(const char* h, uint16_t p, const char* n, const char* t) {
  host = h; port = p; node = n; token = t;
}

void MicroZmq::subscribe(const char* prefix, Callback cb) {
  if (subCount >= MZ_MAX_SUBS) return;
  subs[subCount].prefix = prefix;
  subs[subCount].cb = cb;
  subCount++;
  if (client.connected()) sendFrame(T_SUB, (const uint8_t*)prefix, strlen(prefix));
}

bool MicroZmq::sendFrame(uint8_t type, const uint8_t* body, size_t len) {
  if (len + 1 > MZ_MAX_FRAME || !client.connected()) return false;
  uint8_t out[MZ_MAX_FRAME + 3];
  uint16_t n = len + 1;
  out[0] = n & 0xFF;
  out[1] = n >> 8;
  out[2] = type;
  if (len) memcpy(out + 3, body, len);
  return client.write(out, len + 3) == len + 3;
}

bool MicroZmq::publish(const char* topic, const uint8_t* data, size_t len) {
  size_t tl = strlen(topic);
  if (!ready || tl > 255 || tl + len + 2 > MZ_MAX_FRAME) return false;
  uint8_t body[MZ_MAX_FRAME];
  body[0] = tl;
  memcpy(body + 1, topic, tl);
  if (len) memcpy(body + 1 + tl, data, len);
  return sendFrame(T_PUB, body, 1 + tl + len);
}

void MicroZmq::connectNow() {
  if (!client.connect(host.c_str(), port, 500)) return;   // blocks up to 500 ms, only every 3 s
  client.setNoDelay(true);
  rxLen = 0;
  lastRx = lastPing = millis();

  uint8_t buf[128];
  size_t n = 0;
  for (size_t i = 0; i < token.length() && n < 100; i++) buf[n++] = token[i];
  buf[n++] = 0;
  for (size_t i = 0; i < node.length() && n < 127; i++) buf[n++] = node[i];
  sendFrame(T_HELLO, buf, n);
  for (int i = 0; i < subCount; i++)
    sendFrame(T_SUB, (const uint8_t*)subs[i].prefix.c_str(), subs[i].prefix.length());
}

void MicroZmq::drop() {
  client.stop();
  ready = false;
  rxLen = 0;
}

void MicroZmq::handleFrame(uint8_t type, const uint8_t* body, size_t len) {
  if (type == T_ACK) { ready = true; return; }
  if (type == T_PONG) return;
  if (type == T_PUB && len >= 1) {
    size_t tl = body[0];
    if (tl + 1 > len) return;
    String topic;
    topic.reserve(tl);
    for (size_t i = 0; i < tl; i++) topic += (char)body[1 + i];
    for (int i = 0; i < subCount; i++) {
      if (topic.startsWith(subs[i].prefix)) subs[i].cb(topic, body + 1 + tl, len - 1 - tl);
    }
  }
}

void MicroZmq::update() {
  uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (client.connected()) drop();
    return;
  }
  if (!client.connected()) {
    ready = false;
    if (now - lastTry >= 3000) { lastTry = now; connectNow(); }
    return;
  }

  // read what has arrived
  int avail = client.available();
  if (avail > 0) {
    size_t room = sizeof(rx) - rxLen;
    int n = client.read(rx + rxLen, min((size_t)avail, room));
    if (n > 0) { rxLen += n; lastRx = now; }
  }

  // parse complete frames
  while (rxLen >= 2) {
    uint16_t n = rx[0] | (rx[1] << 8);
    if (n < 1 || n > MZ_MAX_FRAME) { drop(); return; }
    if (rxLen < (size_t)(2 + n)) break;
    handleFrame(rx[2], rx + 3, n - 1);
    memmove(rx, rx + 2 + n, rxLen - 2 - n);
    rxLen -= 2 + n;
  }

  // keepalive
  if (now - lastPing >= 5000) { lastPing = now; sendFrame(T_PING, nullptr, 0); }
  if (now - lastRx > 15000) drop();
}
