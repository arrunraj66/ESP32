#include "distance_module.h"
#if USE_DISTANCE
#include "bus.h"

bool DistanceModule::begin() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  sensor.setBus(&Wire);
  sensor.setTimeout(500);
  if (!sensor.init()) return false;           // sensor not found -> module skipped
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(50000);
  sensor.startContinuous(50);
  return true;
}

void DistanceModule::update() {
  if (!sensor.dataReady()) return;
  uint16_t d = sensor.read(false);
  if (sensor.ranging_data.range_status != VL53L1X::RangeValid) return;

  last3[idx] = d;
  idx = (idx + 1) % 3;

  uint32_t now = millis();
  if (now - lastPublish < 100) return;
  lastPublish = now;

  // median of last 3 samples
  uint16_t a = last3[0], b = last3[1], c = last3[2];
  uint16_t med = max(min(a, b), min(max(a, b), c));
  if (med == 0) return;
  Bus::publish("distance", String(med), name());
}
#endif
