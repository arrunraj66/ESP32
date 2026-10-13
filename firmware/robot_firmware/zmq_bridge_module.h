#pragma once
#include "config.h"
#if USE_ZMQ
#include "module.h"
#include "microzmq.h"

// Connects the local message bus to the ZeroMQ network through the micro-ZMQ agent.
//   bus topic X listed in ZMQ_OUTBOUND_TOPICS  ->  ZeroMQ "robot/out/X"
//   ZeroMQ "robot/in/X"                        ->  bus topic X
// So from any PC node:  publish "robot/in/emotion" "happy"  and the robot's face changes.
class ZmqBridgeModule : public Module {
public:
  const char* name() override { return "zmq"; }
  bool begin() override;
  void update() override;
private:
  MicroZmq mz;
  bool wasWifi = false, wasAgent = false;
};
#endif
