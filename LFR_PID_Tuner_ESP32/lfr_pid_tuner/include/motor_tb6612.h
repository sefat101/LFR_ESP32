#pragma once
#include <Arduino.h>

class MotorTB6612 {
public:
  void begin();
  void stop(uint16_t ms = 0);

  // Target speeds: -255..255 (clamped internally)
  void setTarget(int left, int right);
  void update();

  int outLeft() const { return outL_; }
  int outRight() const { return outR_; }

private:
  void driveOne_(int in1, int in2, int ch, int speed);

  int tgtL_ = 0, tgtR_ = 0;
  int outL_ = 0, outR_ = 0;
};
