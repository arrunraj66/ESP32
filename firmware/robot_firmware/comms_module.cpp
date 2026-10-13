#include "comms_module.h"
#if USE_COMMS
#include "bus.h"

bool CommsModule::begin() {
  Bus::subscribe("*", [](const String& topic, const String& payload, const char* src) {
    if (strcmp(src, "serial") == 0) return;        // don't echo what we just received
    for (size_t i = 0; i < OUTBOUND_COUNT; i++) {
      if (topic == OUTBOUND_TOPICS[i]) {
        Serial.print(topic); Serial.print(' '); Serial.println(payload);
        return;
      }
    }
  });
  return true;
}

void CommsModule::update() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length()) {
        int sp = line.indexOf(' ');
        String topic = (sp < 0) ? line : line.substring(0, sp);
        String payload = (sp < 0) ? "" : line.substring(sp + 1);
        Bus::publish(topic.c_str(), payload, "serial");
        line = "";
      }
    } else if (line.length() < 200) {
      line += c;
    }
  }
}
#endif
