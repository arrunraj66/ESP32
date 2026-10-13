#pragma once
#include "config.h"
#if USE_SERVOS
#include <ESP32Servo.h>
#include "module.h"

// Smooth, limited servo control.
// Bus topics IN:  "servo/cmd"    payload "id,angle[,speed_deg_per_s]"   e.g. "0,45,60"
//                 "servo/center" payload ignored (all servos to center)
class ServoModule : public Module {
public:
  const char* name() override { return "servos"; }
  bool begin() override;
  void update() override;
private:
  Servo servos[NUM_SERVOS];
  float cur[NUM_SERVOS], tgt[NUM_SERVOS], spd[NUM_SERVOS];
  int lastWritten[NUM_SERVOS];
  uint32_t lastMs = 0;
  void setTarget(int id, float angle, float speed);
};
#endif
