#pragma once
#include "config.h"
#if USE_DISPLAY
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "module.h"

// Animated eyes on the 2.8" TFT.
// Bus topics IN: "emotion"   payload: neutral | happy | sad | angry
//                "face/look" payload: "x,y"  each from -1.0 to 1.0
class DisplayModule : public Module {
public:
  const char* name() override { return "display"; }
  bool begin() override;
  void update() override;
private:
  Adafruit_ILI9341* tft = nullptr;
  GFXcanvas16* canvas = nullptr;
  String emotion = "neutral", shownEmotion = "";
  float lookX = 0, lookY = 0, targetX = 0, targetY = 0;
  float openness = 1.0f, shownOpen = -1, shownX = 99, shownY = 99;
  bool blinking = false;
  uint32_t blinkStart = 0, nextBlink = 0, lastFrame = 0;
  void drawEye(int screenX, int screenY, bool leftEye);
};
#endif
