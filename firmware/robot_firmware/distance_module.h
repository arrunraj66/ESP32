#pragma once
#include "config.h"
#if USE_DISTANCE
#include <Wire.h>
#include <VL53L1X.h>     // library: "VL53L1X" by Pololu
#include "module.h"

// VL53L1X time-of-flight sensor.
// Bus topics OUT: "distance" payload in millimetres (median of 3), ~10 Hz
class DistanceModule : public Module {
public:
  const char* name() override { return "distance"; }
  bool begin() override;
  void update() override;
private:
  VL53L1X sensor;
  uint16_t last3[3] = {0, 0, 0};
  int idx = 0;
  uint32_t lastPublish = 0;
};
#endif
