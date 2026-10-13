#pragma once
#include "config.h"
#if USE_BEHAVIOR
#include "module.h"

// Simple "brain" that only uses the bus (no hardware of its own):
//  - idle: looks around randomly (face + head servos)
//  - sees something closer than 400 mm: happy face + beep
//  - nothing near for 3 s: back to neutral
// Replace or extend this module (or move it to the PC) without touching hardware modules.
class BehaviorModule : public Module {
public:
  const char* name() override { return "behavior"; }
  bool begin() override;
  void update() override;
private:
  bool someoneNear = false;
  uint32_t lastNearMs = 0, nextIdleMs = 0;
};
#endif
