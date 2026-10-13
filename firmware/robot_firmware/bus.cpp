#include "bus.h"

namespace {
  struct Sub { String topic; BusHandler handler; };
  const int MAX_SUBS = 40;
  Sub subs[MAX_SUBS];
  int subCount = 0;
}

void Bus::subscribe(const char* topic, BusHandler handler) {
  if (subCount >= MAX_SUBS) { Serial.println("Bus: too many subscribers"); return; }
  subs[subCount].topic = topic;
  subs[subCount].handler = handler;
  subCount++;
}

void Bus::publish(const char* topic, const String& payload, const char* src) {
  for (int i = 0; i < subCount; i++) {
    if (subs[i].topic == "*" || subs[i].topic == topic) {
      subs[i].handler(String(topic), payload, src);
    }
  }
}
