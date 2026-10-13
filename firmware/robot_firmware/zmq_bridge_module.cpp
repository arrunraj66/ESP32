#include "zmq_bridge_module.h"
#if USE_ZMQ
#include "bus.h"

bool ZmqBridgeModule::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                       // lower latency
  WiFi.begin(WIFI_SSID, WIFI_PASS);           // non-blocking: connects in the background

  mz.begin(ZMQ_AGENT_HOST, ZMQ_AGENT_PORT, ZMQ_NODE_NAME, ZMQ_TOKEN);

  // network -> bus
  mz.subscribe("robot/in/", [](const String& topic, const uint8_t* data, size_t len) {
    String busTopic = topic.substring(9);     // strip "robot/in/"
    String payload;
    payload.reserve(len);
    for (size_t i = 0; i < len; i++) payload += (char)data[i];
    Bus::publish(busTopic.c_str(), payload, "zmq");
  });

  // bus -> network
  Bus::subscribe("*", [this](const String& topic, const String& payload, const char* src) {
    if (strcmp(src, "zmq") == 0) return;      // never send back what came from the network
    for (size_t i = 0; i < ZMQ_OUTBOUND_COUNT; i++) {
      if (topic == ZMQ_OUTBOUND_TOPICS[i]) {
        mz.publish((String("robot/out/") + topic).c_str(), payload);
        return;
      }
    }
  });
  return true;
}

void ZmqBridgeModule::update() {
  mz.update();

  bool w = WiFi.status() == WL_CONNECTED;
  if (w != wasWifi) {
    wasWifi = w;
    Serial.println(w ? "[zmq] WiFi connected: " + WiFi.localIP().toString() : "[zmq] WiFi lost");
  }
  bool a = mz.connected();
  if (a != wasAgent) {
    wasAgent = a;
    Serial.println(a ? "[zmq] agent connected" : "[zmq] agent disconnected");
  }
}
#endif
