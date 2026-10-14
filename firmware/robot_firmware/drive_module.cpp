#include "drive_module.h"
#if USE_DRIVE
#include "bus.h"

bool DriveModule::begin() {
  for (int i = 0; i < NUM_WHEELS; i++) {
    wheels[i].setPeriodHertz(50);
    wheels[i].attach(WHEEL_PINS[i], 500, 2500);
    wheels[i].writeMicroseconds(WHEEL_STOP_US[i]);
    delay(100);                                 // stagger start
  }

  Bus::subscribe("drive/cmd", [this](const String&, const String& p, const char*) {
    float x = 0, y = 0;
    if (sscanf(p.c_str(), "%f,%f", &x, &y) == 2) setCmd(x, y);
  });
  Bus::subscribe("drive/stop", [this](const String&, const String&, const char*) {
    for (int i = 0; i < NUM_WHEELS; i++) tgt[i] = 0;
  });
  Bus::subscribe("drive/raw", [this](const String&, const String& p, const char*) {
    float v[NUM_WHEELS];
    if (sscanf(p.c_str(), "%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3]) == 4) {
      for (int i = 0; i < NUM_WHEELS; i++) tgt[i] = constrain(v[i], -1.0f, 1.0f) * DRIVE_MAX_SCALE;
      lastCmdMs = millis();
    }
  });
  Bus::subscribe("drive/trim", [this](const String&, const String& p, const char*) {
    int id = -1, us = 0;
    if (sscanf(p.c_str(), "%d,%d", &id, &us) == 2 && id >= 0 && id < NUM_WHEELS && us > 1300 && us < 1700) {
      WHEEL_STOP_US[id] = us;
      Serial.printf("[drive] wheel %d stop = %d us\n", id, us);
    }
  });
  lastMs = lastCmdMs = millis();
  return true;
}

void DriveModule::setCmd(float x, float y) {
  x = constrain(x, -1.0f, 1.0f);
  y = constrain(y, -1.0f, 1.0f);
  float l = y + x, r = y - x;                   // x only, y=0: opposite directions = spin in place
  float m = max(fabsf(l), fabsf(r));
  if (m > 1.0f) { l /= m; r /= m; }
  for (int i = 0; i < NUM_WHEELS; i++) tgt[i] = (WHEEL_LEFT[i] ? l : r) * DRIVE_MAX_SCALE;
  lastCmdMs = millis();
}

void DriveModule::writeWheel(int i, float v) {
  if (fabsf(v) < 0.02f) v = 0;                  // small dead zone
  if (WHEEL_INVERT[i]) v = -v;
  wheels[i].writeMicroseconds(WHEEL_STOP_US[i] + (int)(v * DRIVE_RANGE_US));
}

void DriveModule::update() {
  uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;

  if (now - lastCmdMs > DRIVE_TIMEOUT_MS)       // link lost or joystick released: stop
    for (int i = 0; i < NUM_WHEELS; i++) tgt[i] = 0;

  float step = DRIVE_RAMP_PER_S * dt;
  for (int i = 0; i < NUM_WHEELS; i++) {
    float d = tgt[i] - cur[i];
    cur[i] = (fabsf(d) <= step) ? tgt[i] : cur[i] + (d > 0 ? step : -step);
    writeWheel(i, cur[i]);
  }
}
#endif
