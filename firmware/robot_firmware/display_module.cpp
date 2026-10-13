#include "display_module.h"
#if USE_DISPLAY
#include <SPI.h>
#include "bus.h"

#define EYE_BOX   130     // size of the square canvas for one eye
#define EYE_W      90
#define EYE_H     100
#define EYE_COLOR ILI9341_CYAN

bool DisplayModule::begin() {
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft = new Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
  tft->begin(40000000);
  tft->setRotation(1);               // 320 x 240 landscape
  tft->fillScreen(ILI9341_BLACK);

  canvas = new GFXcanvas16(EYE_BOX, EYE_BOX);
  if (!canvas || !canvas->getBuffer()) return false;

  Bus::subscribe("emotion", [this](const String&, const String& p, const char*) {
    emotion = p;
  });
  Bus::subscribe("face/look", [this](const String&, const String& p, const char*) {
    float x = 0, y = 0;
    if (sscanf(p.c_str(), "%f,%f", &x, &y) == 2) {
      targetX = constrain(x, -1.0f, 1.0f);
      targetY = constrain(y, -1.0f, 1.0f);
    }
  });

  nextBlink = millis() + 2000;
  return true;
}

void DisplayModule::drawEye(int screenX, int screenY, bool leftEye) {
  canvas->fillScreen(0);
  int cx = EYE_BOX / 2 + (int)(lookX * 18);
  int cy = EYE_BOX / 2 + (int)(lookY * 14);
  int h = max(6, (int)(EYE_H * openness));
  int top = cy - h / 2;
  int left = cx - EYE_W / 2;

  canvas->fillRoundRect(left, top, EYE_W, h, min(30, h / 2), EYE_COLOR);

  // Lids / cheeks drawn in black over the eye to change expression
  int outer = leftEye ? left - 2 : left + EYE_W + 2;
  int inner = leftEye ? left + EYE_W + 2 : left - 2;
  if (emotion == "happy") {
    canvas->fillCircle(cx, cy + h / 2 + 22, 55, 0);                 // cheek curve
  } else if (emotion == "sad") {
    canvas->fillTriangle(left - 2, top - 2, left + EYE_W + 2, top - 2, outer, top + 40, 0);
  } else if (emotion == "angry") {
    canvas->fillTriangle(left - 2, top - 2, left + EYE_W + 2, top - 2, inner, top + 40, 0);
  }

  tft->drawRGBBitmap(screenX, screenY, canvas->getBuffer(), EYE_BOX, EYE_BOX);
}

void DisplayModule::update() {
  uint32_t now = millis();
  if (now - lastFrame < 33) return;   // ~30 fps max
  lastFrame = now;

  // blink
  if (!blinking && now > nextBlink) { blinking = true; blinkStart = now; }
  if (blinking) {
    float t = (now - blinkStart) / 160.0f;
    if (t >= 1.0f) { blinking = false; openness = 1.0f; nextBlink = now + random(2000, 5000); }
    else openness = (t < 0.5f) ? 1.0f - 2 * t : 2 * t - 1.0f;
  }

  // smooth look
  lookX += (targetX - lookX) * 0.25f;
  lookY += (targetY - lookY) * 0.25f;

  bool changed = fabsf(openness - shownOpen) > 0.01f || fabsf(lookX - shownX) > 0.01f ||
                 fabsf(lookY - shownY) > 0.01f || emotion != shownEmotion;
  if (!changed) return;

  drawEye(20, 55, true);
  drawEye(170, 55, false);
  shownOpen = openness; shownX = lookX; shownY = lookY; shownEmotion = emotion;
}
#endif
