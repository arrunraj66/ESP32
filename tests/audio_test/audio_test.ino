// Step 1: INMP441 mic + MAX98357A speaker bench test (ESP32-S3)
// Board package: Arduino-ESP32 2.0.17 (legacy I2S driver)
// Board: ESP32S3 Dev Module, USB CDC On Boot: Enabled, Serial 115200

#include <driver/i2s.h>
#include <math.h>

#define SAMPLE_RATE 16000

// Mic (INMP441) on I2S port 0
#define MIC_PORT I2S_NUM_0
#define MIC_SCK  4
#define MIC_WS   5
#define MIC_SD   6

// Speaker amp (MAX98357A) on I2S port 1
#define SPK_PORT I2S_NUM_1
#define SPK_BCLK 7
#define SPK_LRC  15
#define SPK_DIN  16

void setupMic() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;      // INMP441 gives 24-bit in 32-bit slot
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;       // L/R pin tied to GND
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

void setupSpeaker() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;      // stereo frames, amp mixes to mono
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

// Play a sine tone (keep amplitude low at first!)
void beep(float freq, int ms, int16_t amplitude = 3000) {
  int total = SAMPLE_RATE * ms / 1000;
  int16_t frame[2];
  size_t written;
  for (int i = 0; i < total; i++) {
    int16_t s = (int16_t)(amplitude * sinf(2.0f * PI * freq * i / SAMPLE_RATE));
    frame[0] = s;  // left
    frame[1] = s;  // right
    i2s_write(SPK_PORT, frame, sizeof(frame), &written, portMAX_DELAY);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Audio test starting...");

  setupSpeaker();
  setupMic();

  Serial.println("Playing beeps...");
  beep(440, 300);
  beep(660, 300);
  beep(880, 300);
  Serial.println("Now reading mic. Clap or talk!");
}

void loop() {
  int32_t buf[256];
  size_t bytesRead = 0;
  i2s_read(MIC_PORT, buf, sizeof(buf), &bytesRead, portMAX_DELAY);

  int n = bytesRead / sizeof(int32_t);
  if (n == 0) return;

  // Peak level, 24-bit data sits in the top bits of each 32-bit word
  int32_t peak = 0;
  for (int i = 0; i < n; i++) {
    int32_t v = buf[i] >> 14;       // scale down to roughly 18-bit
    if (v < 0) v = -v;
    if (v > peak) peak = v;
  }

  // Simple bar graph + number
  int bars = constrain(peak / 500, 0, 40);
  Serial.printf("%6ld ", (long)peak);
  for (int i = 0; i < bars; i++) Serial.print('#');
  Serial.println();
  delay(50);
}
