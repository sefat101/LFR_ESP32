#include "config.h"
#include "motor_tb6612.h"

static inline int applyMinMove(int v) {
  if (v == 0) return 0;
  int s = (v > 0) ? 1 : -1;
  int a = abs(v);
  if (a < MIN_MOVE) a = MIN_MOVE;
  return s * a;
}

static inline int slewStep(int cur, int tgt, int step) {
  if (cur < tgt) return min(cur + step, tgt);
  if (cur > tgt) return max(cur - step, tgt);
  return cur;
}

void MotorTB6612::begin() {
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT); pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT); pinMode(PIN_BIN2, OUTPUT);

  ledcSetup(CH_LEFT,  PWM_FREQ, PWM_RES);
  ledcSetup(CH_RIGHT, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_PWMA, CH_LEFT);
  ledcAttachPin(PIN_PWMB, CH_RIGHT);

  digitalWrite(PIN_STBY, HIGH);
  stop(0);
}

void MotorTB6612::stop(uint16_t ms) {
  tgtL_ = tgtR_ = 0;
  outL_ = outR_ = 0;
  driveOne_(PIN_AIN1, PIN_AIN2, CH_LEFT, 0);
  driveOne_(PIN_BIN1, PIN_BIN2, CH_RIGHT, 0);
  if (ms) delay(ms);
}

void MotorTB6612::setTarget(int left, int right) {
  left  = constrain(left  * MOTOR_SIGN_L, -PWM_MAX, PWM_MAX);
  right = constrain(right * MOTOR_SIGN_R, -PWM_MAX, PWM_MAX);
  tgtL_ = left;
  tgtR_ = right;
}

void MotorTB6612::update() {
  digitalWrite(PIN_STBY, HIGH);

  // Smooth acceleration (slew limiting)
  outL_ = slewStep(outL_, tgtL_, 10);
  outR_ = slewStep(outR_, tgtR_, 10);

  int l = applyMinMove(outL_);
  int r = applyMinMove(outR_);

  driveOne_(PIN_AIN1, PIN_AIN2, CH_LEFT,  l);
  driveOne_(PIN_BIN1, PIN_BIN2, CH_RIGHT, r);
}

void MotorTB6612::driveOne_(int in1, int in2, int ch, int speed) {
  speed = constrain(speed, -PWM_MAX, PWM_MAX);

  if (speed > 0) {
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
    ledcWrite(ch, speed);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);  digitalWrite(in2, HIGH);
    ledcWrite(ch, -speed);
  } else {
    digitalWrite(in1, LOW);  digitalWrite(in2, LOW);
    ledcWrite(ch, 0);
  }
}
