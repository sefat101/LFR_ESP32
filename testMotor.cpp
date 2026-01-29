#include <Arduino.h>

// ================== TB6612FNG -> ESP32 Dev Module ==================
#define STBY_PIN 23

// Motor A (Left)
#define AIN1 18
#define AIN2 19
#define PWMA 25   // PWM

// Motor B (Right)
#define BIN1 16
#define BIN2 17
#define PWMB 26   // PWM

// ================== PWM settings ==================
const uint32_t PWM_FREQ = 20000;  // 20kHz
const uint8_t  PWM_RES  = 8;      // 0..255
const int      PWM_MAX  = (1 << PWM_RES) - 1;

const int CH_A = 0;
const int CH_B = 1;

// Flip if direction is reversed
int MOTOR_SIGN_L = +1;
int MOTOR_SIGN_R = +1;

static inline void pwmWriteCh(int ch, int duty) {
  duty = constrain(duty, 0, PWM_MAX);
  ledcWrite(ch, duty);
}

void motorWrite(int in1, int in2, int pwmCh, int speed) {
  speed = constrain(speed, -PWM_MAX, PWM_MAX);

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    pwmWriteCh(pwmCh, speed);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    pwmWriteCh(pwmCh, -speed);
  } else {
    // coast
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    pwmWriteCh(pwmCh, 0);
  }
}

void setMotors(int left, int right) {
  digitalWrite(STBY_PIN, HIGH);
  motorWrite(AIN1, AIN2, CH_A, left  * MOTOR_SIGN_L);
  motorWrite(BIN1, BIN2, CH_B, right * MOTOR_SIGN_R);
}

void stopMotors(int ms = 300) {
  setMotors(0, 0);
  delay(ms);
}

void setup() {
  Serial.begin(115200);

  pinMode(STBY_PIN, OUTPUT);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  // PWM init for ESP32 (WROOM-32)
  ledcSetup(CH_A, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWMA, CH_A);
  ledcAttachPin(PWMB, CH_B);

  stopMotors(200);
  Serial.println("TB6612FNG + N20 | ESP32 Dev Module Motor Test Start");
}

void loop() {
  Serial.println("Forward");
  setMotors(180, 180);
  delay(2000);
  stopMotors();

  Serial.println("Reverse");
  setMotors(-180, -180);
  delay(2000);
  stopMotors();

  Serial.println("Spin Left");
  setMotors(-180, 180);
  delay(1500);
  stopMotors();

  Serial.println("Spin Right");
  setMotors(180, -180);
  delay(1500);
  stopMotors();

  Serial.println("Ramp Up");
  for (int s = 0; s <= 220; s += 20) {
    setMotors(s, s);
    delay(200);
  }

  Serial.println("Ramp Down");
  for (int s = 220; s >= 0; s -= 20) {
    setMotors(s, s);
    delay(200);
  }

  stopMotors(800);
}
