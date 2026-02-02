#pragma once
#include <Arduino.h>

struct PIDGains {
  float kp, ki, kd;
};

class PIDController {
public:
  void reset();
  float update(float error, float dt, const PIDGains &g);

private:
  float integral_ = 0;
  float lastErr_  = 0;
};
