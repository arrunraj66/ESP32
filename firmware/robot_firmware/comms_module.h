#pragma once
#include "config.h"
#if USE_COMMS
#include "module.h"

// Serial transport. Type a line "topic payload" in the Serial Monitor (115200):
//   emotion happy
//   servo/cmd 0,45,60
//   audio/beep 880,200
//   face/look 0.5,-0.3
// Topics listed in OUTBOUND_TOPICS (config.h) are printed back as "topic payload".
// To add WiFi / WebSocket / ZeroMQ-bridge later: copy this module, change only
// how lines are received and sent. All other modules stay the same.
class CommsModule : public Module {
public:
  const char* name() override { return "comms"; }
  bool begin() override;
  void update() override;
private:
  String line;
};
#endif
