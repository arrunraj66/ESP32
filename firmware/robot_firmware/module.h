#pragma once

// Base class every module implements.
// begin()  : set up hardware + subscribe to bus topics. Return false if hardware missing.
// update() : called every loop. Must be NON-BLOCKING (no long delay()).
class Module {
public:
  virtual const char* name() = 0;
  virtual bool begin() = 0;
  virtual void update() = 0;
  virtual ~Module() {}
};
