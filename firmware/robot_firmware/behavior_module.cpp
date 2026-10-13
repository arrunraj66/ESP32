#include "behavior_module.h"
#if USE_BEHAVIOR
#include "bus.h"

#define NEAR_MM 400

bool BehaviorModule::begin() {
  Bus::subscribe("distance", [this](const String&, const String& p, const char*) {
    int d = p.toInt();
    if (d <= 0 || d > NEAR_MM) return;
    lastNearMs = millis();
    if (!someoneNear) {
      someoneNear = true;
      Bus::publish("emotion", "happy", name());
      Bus::publish("face/look", "0,0", name());
      Bus::publish("servo/cmd", String(SERVO_PAN) + ",90,120", name());
      Bus::publish("servo/cmd", String(SERVO_TILT) + ",90,120", name());
      Bus::publish("audio/beep", "880,120", name());
    }
  });
  nextIdleMs = millis() + 3000;
  return true;
}

void BehaviorModule::update() {
  uint32_t now = millis();

  if (someoneNear && now - lastNearMs > 3000) {
    someoneNear = false;
    Bus::publish("emotion", "neutral", name());
  }

  if (!someoneNear && now > nextIdleMs) {
    float x = random(-100, 101) / 100.0f;
    float y = random(-100, 101) / 100.0f;
    Bus::publish("face/look", String(x, 2) + "," + String(y, 2), name());
    Bus::publish("servo/cmd", String(SERVO_PAN) + "," + String(90 + x * 20, 0) + ",40", name());
    Bus::publish("servo/cmd", String(SERVO_TILT) + "," + String(90 + y * 10, 0) + ",40", name());
    nextIdleMs = now + random(2500, 6000);
  }
}
#endif
