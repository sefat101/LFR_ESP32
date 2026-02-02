#include <Arduino.h>
#include "config.h"
#include "motor_tb6612.h"
#include "sensor_mux12.h"
#include "pid_controller.h"
#include "wifi_tuner.h"

MotorTB6612 motors;
SensorMux12  sensors;
PIDController pid;
WifiTuner wifi;

enum Mode { MODE_CALIB, MODE_RUN, MODE_STOP };
volatile Mode mode = MODE_CALIB;

PIDGains gains { KP_DEFAULT, KI_DEFAULT, KD_DEFAULT };
LiveStatus st;

volatile bool reqCalib = false;
volatile bool reqStop  = false;
volatile bool reqRun   = false;

uint32_t calibStartMs = 0;
uint32_t lastLoopUs   = 0;

static void enterCalib() {
  mode = MODE_CALIB;
  calibStartMs = millis();
  sensors.resetCalibration();
  pid.reset();
  st.lost = 0;
  st.err = 0;
}

static void enterRun() {
  mode = MODE_RUN;
  pid.reset();
  st.lost = 0;
  lastLoopUs = micros();
}

static void enterStop() {
  mode = MODE_STOP;
  motors.stop(0);
}

void setup() {
  Serial.begin(115200);

  motors.begin();
  sensors.begin();
  pid.reset();

  wifi.begin(&gains, &st, &reqCalib, &reqStop, &reqRun);

  delay(1200);
  enterCalib();
}

void loop() {
  wifi.loop();

  if (reqCalib) { reqCalib = false; enterCalib(); }
  if (reqStop)  { reqStop  = false; enterStop();  }
  if (reqRun)   { reqRun   = false; enterRun();   }

  sensors.readRaw();

  uint32_t nowUs = micros();
  float dt = (nowUs - lastLoopUs) / 1000000.0f;
  if (dt <= 0) dt = 0.001f;
  lastLoopUs = nowUs;

  // ---- CALIBRATION ----
  if (mode == MODE_CALIB) {
    bool dir = ((millis() - calibStartMs) / 250) % 2;
    motors.setTarget(dir ? -95 : 95, dir ? 95 : -95);
    motors.update();

    sensors.calibrateStep();

    if (millis() - calibStartMs >= CALIB_MS) {
      motors.stop(150);
      sensors.finishCalibration();
      enterRun();
    }
    return;
  }

  // ---- STOP ----
  if (mode == MODE_STOP) {
    motors.setTarget(0, 0);
    motors.update();
    st.outL = motors.outLeft();
    st.outR = motors.outRight();
    return;
  }

  // ---- RUN ----
  sensors.updateFiltered();

  int pos = 5500, sum = 0;
  bool ok = sensors.linePosition(pos, sum);

  st.pos = pos;
  st.sum = sum;

  if (!ok) {
    st.lost++;

    int seek = 120 + min(st.lost * 3, 140);
    int fwd  = 80;

    int left  = fwd + ((st.err >= 0) ? +seek : -seek);
    int right = fwd + ((st.err >= 0) ? -seek : +seek);

    if (st.lost > LOST_STOP_COUNT) motors.setTarget(0, 0);
    else motors.setTarget(left, right);

    motors.update();
    st.outL = motors.outLeft();
    st.outR = motors.outRight();
    delay(2);
    return;
  }

  st.lost = 0;

  float err = (float)(pos - 5500);
  st.err = err;

  float corr = pid.update(err, dt, gains);

  float turnMag = min(fabs(err) / 5500.0f, 1.0f);
  int dynBase = (int)(BASE_SPEED * (1.0f - TURN_SLOW * turnMag));
  dynBase = constrain(dynBase, 90, BASE_SPEED);

  int left  = (int)(dynBase + corr);
  int right = (int)(dynBase - corr);

  left  = constrain(left,  -MAX_SPEED, MAX_SPEED);
  right = constrain(right, -MAX_SPEED, MAX_SPEED);

  motors.setTarget(left, right);
  motors.update();

  st.outL = motors.outLeft();
  st.outR = motors.outRight();

  delay(2);
}
