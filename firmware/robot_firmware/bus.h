#pragma once
#include <Arduino.h>
#include <functional>

// Tiny publish/subscribe message bus.
// Topic = text like "emotion", payload = text like "happy" or "0,45,60".
// Subscribe to "*" to receive every message.
// This is the only thing modules use to talk to each other, so the same
// topics can later be mapped to ZeroMQ / protobuf messages by a bridge module.

typedef std::function<void(const String& topic, const String& payload, const char* src)> BusHandler;

namespace Bus {
  void subscribe(const char* topic, BusHandler handler);
  void publish(const char* topic, const String& payload, const char* src = "");
}
