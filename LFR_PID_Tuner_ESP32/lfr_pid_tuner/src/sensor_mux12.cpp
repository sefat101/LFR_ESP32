#include "config.h"
#include "sensor_mux12.h"

void SensorMux12::begin() {
  pinMode(PIN_SEL_A, OUTPUT);
  pinMode(PIN_SEL_B, OUTPUT);
  analogReadResolution(12);
  resetCalibration();
}

void SensorMux12::resetCalibration() {
  for (int i = 0; i < SENSOR_N; i++) {
    mn_[i] = 4095;
    mx_[i] = 0;
    filt_[i] = 0.0f;
  }
}

int SensorMux12::readStable_(int pin) const {
  (void)analogRead(pin);
  int a = analogRead(pin);
  int b = analogRead(pin);
  return (a + b) / 2;
}

void SensorMux12::readRaw() {
  // Mapping matches your mux wiring / board routing

  // Select = 00
  digitalWrite(PIN_SEL_A, LOW);
  digitalWrite(PIN_SEL_B, LOW);
  delayMicroseconds(5);
  raw_[6] = readStable_(PIN_IN_A);
  raw_[4] = readStable_(PIN_IN_C);
  raw_[1] = readStable_(PIN_IN_D);

  // Select = 01
  digitalWrite(PIN_SEL_A, LOW);
  digitalWrite(PIN_SEL_B, HIGH);
  delayMicroseconds(5);
  raw_[7]  = readStable_(PIN_IN_A);
  raw_[11] = readStable_(PIN_IN_B);
  raw_[2]  = readStable_(PIN_IN_C);
  raw_[0]  = readStable_(PIN_IN_D);

  // Select = 10
  digitalWrite(PIN_SEL_A, HIGH);
  digitalWrite(PIN_SEL_B, LOW);
  delayMicroseconds(5);
  raw_[9]  = readStable_(PIN_IN_A);
  raw_[10] = readStable_(PIN_IN_B);
  raw_[3]  = readStable_(PIN_IN_C);

  // Select = 11
  digitalWrite(PIN_SEL_A, HIGH);
  digitalWrite(PIN_SEL_B, HIGH);
  delayMicroseconds(5);
  raw_[8] = readStable_(PIN_IN_A);
  raw_[5] = readStable_(PIN_IN_C);
}

void SensorMux12::calibrateStep() {
  for (int i = 0; i < SENSOR_N; i++) {
    int v = raw_[i];
    if (v < mn_[i]) mn_[i] = v;
    if (v > mx_[i]) mx_[i] = v;
  }
}

void SensorMux12::finishCalibration() {
  for (int i = 0; i < SENSOR_N; i++) {
    if (mx_[i] - mn_[i] < 30) mx_[i] = mn_[i] + 30;
  }
}

void SensorMux12::updateFiltered() {
  for (int i = 0; i < SENSOR_N; i++) {
    int mn = mn_[i], mx = mx_[i], v = raw_[i];
    int n = 0;
    int range = mx - mn;
    if (range >= 30) {
      n = (int)(((long)(v - mn) * 1000L) / (long)range);
    }
    n = constrain(n, 0, 1000);

    float x = (float)n;
    filt_[i] = (1.0f - SENSOR_ALPHA) * x + SENSOR_ALPHA * filt_[i];
  }
}

static inline int lineStrength(int normVal) {
  // Your sensor: black/dark => HIGH -> strength = normVal for black line
  return LINE_IS_BLACK ? normVal : (1000 - normVal);
}

bool SensorMux12::linePosition(int &pos, int &sum) const {
  int li[SENSOR_N];

  int mnL = 1000, mxL = 0;
  for (int i = 0; i < SENSOR_N; i++) {
    li[i] = lineStrength((int)filt_[i]);
    mnL = min(mnL, li[i]);
    mxL = max(mxL, li[i]);
  }

  int contrast = mxL - mnL;
  if (contrast < CONTRAST_TH) {
    sum = 0;
    return false; // uniform dark/bright -> reject
  }

  int thr = mnL + (int)(TH_FRAC * (float)contrast);

  long wSum = 0;
  long sSum = 0;
  int active = 0;

  for (int i = 0; i < SENSOR_N; i++) {
    int s = li[i] - thr;
    if (s < 0) s = 0;
    if (s > 0) active++;
    sSum += s;
    wSum += (long)s * (long)(i * 1000);
  }

  sum = (int)sSum;
  if (active == 0 || sSum < LINE_SUM_TH) return false;

  pos = (int)(wSum / sSum); // 0..11000 (center ~5500)
  return true;
}

void SensorMux12::getFiltered(int out12[]) const {
  for (int i = 0; i < SENSOR_N; i++) out12[i] = (int)filt_[i];
}
