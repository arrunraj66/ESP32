// Desktop Robot firmware (ESP32-S3)
// Arduino-ESP32 core 2.0.17  |  Board: ESP32S3 Dev Module  |  USB CDC On Boot: Enabled
// Libraries: ESP32Servo, Adafruit GFX Library, Adafruit ILI9341, VL53L1X (Pololu)
//
// HOW TO ADD A NEW MODULE
//   1. Copy any *_module.h/.cpp, rename the class, subscribe to topics in begin(),
//      do non-blocking work in update(), publish results with Bus::publish().
//   2. Add one #include and one addModule(new YourModule()) line below.

#include "config.h"
#include "bus.h"
#include "module.h"

#include "comms_module.h"
#include "servo_module.h"
#include "drive_module.h"
#include "display_module.h"
#include "audio_module.h"
#include "distance_module.h"
#include "behavior_module.h"
#include "zmq_bridge_module.h"

#define MAX_MODULES 12
static Module* modules[MAX_MODULES];
static bool active[MAX_MODULES];
static int moduleCount = 0;

static void addModule(Module* m) {
  if (moduleCount < MAX_MODULES) {
    modules[moduleCount] = m;
    active[moduleCount] = false;
    moduleCount++;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Desktop Robot ===");

#if USE_COMMS
  addModule(new CommsModule());
#endif
#if USE_ZMQ
  addModule(new ZmqBridgeModule());
#endif
#if USE_SERVOS
  addModule(new ServoModule());
#endif
#if USE_DRIVE
  addModule(new DriveModule());
#endif
#if USE_DISPLAY
  addModule(new DisplayModule());
#endif
#if USE_AUDIO
  addModule(new AudioModule());
#endif
#if USE_DISTANCE
  addModule(new DistanceModule());
#endif
#if USE_BEHAVIOR
  addModule(new BehaviorModule());
#endif

  for (int i = 0; i < moduleCount; i++) {
    active[i] = modules[i]->begin();
    Serial.printf("[%s] %s\n", modules[i]->name(), active[i] ? "OK" : "FAILED (skipped)");
  }

  Bus::publish("emotion", "happy", "main");
  Bus::publish("audio/beep", "660,150", "main");
  Serial.println("Ready. Type e.g.:  emotion sad   |   servo/cmd 0,45,60   |   audio/beep 880,200");
}

void loop() {
  for (int i = 0; i < moduleCount; i++) {
    if (active[i]) modules[i]->update();
  }
}
