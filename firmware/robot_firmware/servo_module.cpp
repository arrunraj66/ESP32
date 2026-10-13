#include "servo_module.h"
#if USE_SERVOS
#include "bus.h"

bool ServoModule::begin() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < NUM_SERVOS; i++) {
    cur[i] = tgt[i] = SERVO_CENTER[i];
    spd[i] = 90;
    servos[i].setPeriodHertz(50);
    servos[i].attach(SERVO_PINS[i], 500, 2400);
    servos[i].write(SERVO_CENTER[i]);
    lastWritten[i] = SERVO_CENTER[i];
    delay(150);  // stagger start so servos don't all draw current at once
  }

  Bus::subscribe("servo/cmd", [this](const String&, const String& p, const char*) {
    int id = -1; float angle = 90, speed = 90;
    int n = sscanf(p.c_str(), "%d,%f,%f", &id, &angle, &speed);
    if (n >= 2) setTarget(id, angle, n >= 3 ? speed : 90);
  });

  Bus::subscribe("servo/center", [this](const String&, const String&, const char*) {
    for (int i = 0; i < NUM_SERVOS; i++) setTarget(i, SERVO_CENTER[i], 90);
  });

  lastMs = millis();
  return true;
}

void ServoModule::setTarget(int id, float angle, float speed) {
  if (id < 0 || id >= NUM_SERVOS) return;
  tgt[id] = constrain(angle, (float)SERVO_MIN[id], (float)SERVO_MAX[id]);
  spd[id] = max(speed, 1.0f);
}

void ServoModule::update() {
  uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;

  for (int i = 0; i < NUM_SERVOS; i++) {
    float diff = tgt[i] - cur[i];
    if (fabsf(diff) > 0.1f) {
      float step = spd[i] * dt;
      cur[i] = (fabsf(diff) <= step) ? tgt[i] : cur[i] + (diff > 0 ? step : -step);
    }
    int a = (int)roundf(cur[i]);
    if (a != lastWritten[i]) {
      servos[i].write(a);
      lastWritten[i] = a;
    }
  }
}
#endif
