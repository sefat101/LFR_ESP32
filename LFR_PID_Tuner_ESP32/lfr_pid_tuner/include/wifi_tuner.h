#pragma once
#include <Arduino.h>
#include "pid_controller.h"

struct LiveStatus {
  int pos = 5500;
  int sum = 0;
  int lost = 0;
  float err = 0;
  int outL = 0;
  int outR = 0;
};

class WifiTuner {
public:
  void begin(PIDGains *gains, LiveStatus *status,
             volatile bool *reqCalib, volatile bool *reqStop, volatile bool *reqRun);
  void loop();

private:
  void setupRoutes_();
  String page_() const;

  PIDGains *g_ = nullptr;
  LiveStatus *st_ = nullptr;
  volatile bool *reqCalib_ = nullptr;
  volatile bool *reqStop_  = nullptr;
  volatile bool *reqRun_   = nullptr;
};
