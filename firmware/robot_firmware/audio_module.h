#pragma once
#include "config.h"
#if USE_AUDIO
#include "module.h"

// Mic level + non-blocking speaker beeps.
// Bus topics IN:  "audio/beep"  payload "freq_hz,duration_ms"   e.g. "880,200"
// Bus topics OUT: "audio/level" payload peak level, every 200 ms
//                 "audio/loud"  payload peak level, when a clap/loud sound is heard
class AudioModule : public Module {
public:
  const char* name() override { return "audio"; }
  bool begin() override;
  void update() override;
private:
  float beepFreq = 0;
  int beepTotal = 0, beepPos = 0;
  int32_t windowPeak = 0;
  uint32_t lastLevelMs = 0, lastLoudMs = 0;
};
#endif
