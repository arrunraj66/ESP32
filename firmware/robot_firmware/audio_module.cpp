#include "audio_module.h"
#if USE_AUDIO
#include "bus.h"
#include <driver/i2s.h>
#include <math.h>

#define MIC_PORT I2S_NUM_0
#define SPK_PORT I2S_NUM_1

static void setupMic() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = AUDIO_SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;      // INMP441 L/R pin -> GND
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = MIC_SCK;
  pins.ws_io_num = MIC_WS;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = MIC_SD;
  i2s_driver_install(MIC_PORT, &cfg, 0, NULL);
  i2s_set_pin(MIC_PORT, &pins);
}

static void setupSpeaker() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = AUDIO_SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;
  cfg.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = SPK_BCLK;
  pins.ws_io_num = SPK_LRC;
  pins.data_out_num = SPK_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  i2s_driver_install(SPK_PORT, &cfg, 0, NULL);
  i2s_set_pin(SPK_PORT, &pins);
}

bool AudioModule::begin() {
  setupSpeaker();
  setupMic();

  Bus::subscribe("audio/beep", [this](const String&, const String& p, const char*) {
    float f = 0; int ms = 0;
    if (sscanf(p.c_str(), "%f,%d", &f, &ms) == 2 && f > 0 && ms > 0) {
      beepFreq = f;
      beepTotal = AUDIO_SAMPLE_RATE / 1000 * ms;
      beepPos = 0;
    }
  });
  return true;
}

void AudioModule::update() {
  // ---- speaker: write a small chunk of the beep each loop ----
  if (beepPos < beepTotal) {
    int16_t frames[256];                       // 128 stereo frames
    int n = min(128, beepTotal - beepPos);
    for (int i = 0; i < n; i++) {
      int16_t s = (int16_t)(BEEP_AMPLITUDE * sinf(2.0f * PI * beepFreq * (beepPos + i) / AUDIO_SAMPLE_RATE));
      frames[2 * i] = s;
      frames[2 * i + 1] = s;
    }
    size_t w = 0;
    i2s_write(SPK_PORT, frames, n * 4, &w, 0);
    beepPos += w / 4;
  }

  // ---- mic: read what is available, track peak ----
  int32_t buf[128];
  size_t br = 0;
  i2s_read(MIC_PORT, buf, sizeof(buf), &br, 0);
  int n = br / sizeof(int32_t);
  for (int i = 0; i < n; i++) {
    int32_t v = buf[i] >> 14;
    if (v < 0) v = -v;
    if (v > windowPeak) windowPeak = v;
  }

  uint32_t now = millis();
  if (now - lastLevelMs >= 200) {
    lastLevelMs = now;
    Bus::publish("audio/level", String(windowPeak), name());
    if (windowPeak > AUDIO_LOUD_THRESHOLD && now - lastLoudMs > 1000) {
      lastLoudMs = now;
      Bus::publish("audio/loud", String(windowPeak), name());
    }
    windowPeak = 0;
  }
}
#endif
