#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <functional>

// micro-ZMQ client for ESP32 (see PROTOCOL.md). Non-blocking, auto-reconnect, keepalive.
//
//   MicroZmq mz;
//   mz.begin("192.168.1.50", 7400, "robot-body", "token");
//   mz.subscribe("robot/in/", [](const String& topic, const uint8_t* d, size_t n){ ... });
//   mz.publish("robot/out/distance", "321");
//   loop(): mz.update();

#define MZ_MAX_FRAME 1024
#define MZ_MAX_SUBS  8

class MicroZmq {
public:
  typedef std::function<void(const String& topic, const uint8_t* data, size_t len)> Callback;

  void begin(const char* host, uint16_t port, const char* node, const char* token);
  void subscribe(const char* prefix, Callback cb);
  bool publish(const char* topic, const uint8_t* data, size_t len);
  bool publish(const char* topic, const String& s) {
    return publish(topic, (const uint8_t*)s.c_str(), s.length());
  }
  bool connected() const { return ready; }
  void update();   // call every loop

private:
  WiFiClient client;
  String host, node, token;
  uint16_t port = 0;
  bool ready = false;
  uint8_t rx[MZ_MAX_FRAME + 2];
  size_t rxLen = 0;
  uint32_t lastTry = 0, lastPing = 0, lastRx = 0;

  struct Sub { String prefix; Callback cb; };
  Sub subs[MZ_MAX_SUBS];
  int subCount = 0;

  bool sendFrame(uint8_t type, const uint8_t* body, size_t len);
  void connectNow();
  void drop();
  void handleFrame(uint8_t type, const uint8_t* body, size_t len);
};
