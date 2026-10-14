#pragma once
#include "config.h"
#if USE_DRIVE
#include <ESP32Servo.h>
#include "module.h"

// Skid-steer drive with 4 continuous-rotation servos.
// Bus topics IN:  "drive/cmd"  payload "x,y"  x = turn (right +), y = forward (+), both -1..1
//                              y=0 and x!=0 spins the robot in place.
//                 "drive/stop" payload ignored
//                 "drive/raw"  payload "fl,rl,fr,rr" each -1..1, for testing one wheel at a time
//                 "drive/trim" payload "wheel,us" sets that wheel's stop pulse (RAM only)
// Stops by itself if no command arrives for DRIVE_TIMEOUT_MS.
class DriveModule : public Module {
public:
  const char* name() override { return "drive"; }
  bool begin() override;
  void update() override;
private:
  Servo wheels[NUM_WHEELS];
  float tgt[NUM_WHEELS] = {0}, cur[NUM_WHEELS] = {0};
  uint32_t lastCmdMs = 0, lastMs = 0;
  void setCmd(float x, float y);
  void writeWheel(int i, float v);
};
#endif
