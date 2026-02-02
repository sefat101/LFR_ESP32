#include "pid_controller.h"

void PIDController::reset() {
  integral_ = 0;
  lastErr_  = 0;
}

float PIDController::update(float error, float dt, const PIDGains &g) {
  if (dt <= 0) dt = 0.001f;

  integral_ += error * dt;
  integral_ = constrain(integral_, -9000.0f, 9000.0f);

  float der = (error - lastErr_) / dt;
  lastErr_ = error;

  return g.kp * error + g.ki * integral_ + g.kd * der;
}
