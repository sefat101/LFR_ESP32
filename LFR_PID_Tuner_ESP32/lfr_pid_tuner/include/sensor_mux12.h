#pragma once
#include <Arduino.h>

class SensorMux12 {
public:
  void begin();
  void readRaw();
  void resetCalibration();
  void calibrateStep();
  void finishCalibration();
  void updateFiltered();

  // Returns true if line detected; pos: 0..11000, center=5500
  bool linePosition(int &pos, int &sum) const;

  void getFiltered(int out12[]) const;

private:
  int readStable_(int pin) const;

  int raw_[12]{};
  int mn_[12]{}, mx_[12]{};
  float filt_[12]{};
};
