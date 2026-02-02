#pragma once
#include <Arduino.h>

// =============================================================
// Project: ESP32 LFR + TB6612FNG + 12ch MUX Visible Light Sensors
// Features: Modular code + SoftAP Wi-Fi PID tuning dashboard
// Note: Your sensor polarity: black/dark => ADC HIGH
// =============================================================

// ===================== PINS: TB6612FNG =====================
static constexpr int PIN_STBY = 23;

// Left motor (A)
static constexpr int PIN_AIN1 = 18;
static constexpr int PIN_AIN2 = 19;
static constexpr int PIN_PWMA = 25;

// Right motor (B)
static constexpr int PIN_BIN1 = 16;
static constexpr int PIN_BIN2 = 17;
static constexpr int PIN_PWMB = 26;

// PWM (LEDC)
static constexpr uint32_t PWM_FREQ = 20000;
static constexpr uint8_t  PWM_RES  = 8;
static constexpr int      PWM_MAX  = (1 << PWM_RES) - 1;
static constexpr int      CH_LEFT  = 0;
static constexpr int      CH_RIGHT = 1;

// Motor direction (flip if needed)
static constexpr int MOTOR_SIGN_L = +1;
static constexpr int MOTOR_SIGN_R = +1;

// ===================== PINS: 12ch MUX SENSOR =====================
static constexpr int PIN_SEL_A = 32;
static constexpr int PIN_SEL_B = 33;

// ADC1 pins (Wi-Fi safe)
static constexpr int PIN_IN_A = 36;
static constexpr int PIN_IN_B = 39;
static constexpr int PIN_IN_C = 34;
static constexpr int PIN_IN_D = 35;

static constexpr int SENSOR_N = 12;

// ===================== SENSOR BEHAVIOR =====================
// You measured: black/dark => ADC HIGH, white/bright => ADC LOW
static constexpr bool LINE_IS_BLACK = true;  // black tape on lighter floor

// Filtering
static constexpr float SENSOR_ALPHA = 0.35f;

// Auto calibration time
static constexpr uint32_t CALIB_MS = 2500;

// Robust line detect (good for ~3cm line)
static constexpr int   CONTRAST_TH = 80;     // false detect -> 100/120
static constexpr float TH_FRAC     = 0.65f;  // if line lost too much -> 0.60
static constexpr int   LINE_SUM_TH = 120;    // 80..200 depending on reflectance

// ===================== CONTROL =====================
static constexpr int   BASE_SPEED = 155;
static constexpr int   MAX_SPEED  = 230;
static constexpr int   MIN_MOVE   = 55;
static constexpr float TURN_SLOW  = 0.45f;

// PID defaults (live tunable)
static constexpr float KP_DEFAULT = 0.060f;
static constexpr float KI_DEFAULT = 0.0000f;
static constexpr float KD_DEFAULT = 0.130f;

// Steps you requested (live)
static constexpr float KP_STEP = 0.5f;   // NOTE: large step; consider 0.05 for fine tuning
static constexpr float KI_STEP = 0.01f;
static constexpr float KD_STEP = 0.01f;

// Clamp ranges (safety)
static constexpr float KP_MIN = 0.0f,  KP_MAX = 5.0f;
static constexpr float KI_MIN = 0.0f,  KI_MAX = 1.0f;
static constexpr float KD_MIN = 0.0f,  KD_MAX = 5.0f;

// Line lost
static constexpr int LOST_STOP_COUNT = 70;

// ===================== Wi-Fi (SoftAP) =====================
// No router needed. Connect your phone to this Wi-Fi, open http://192.168.4.1
static constexpr const char* WIFI_AP_SSID = "LFR-Tuner";
static constexpr const char* WIFI_AP_PASS = "12345678"; // >=8 chars required

// Optional: also try STA (router). Keep false for simplicity.
static constexpr bool WIFI_TRY_STA = false;
static constexpr const char* WIFI_STA_SSID = "YOUR_WIFI";
static constexpr const char* WIFI_STA_PASS = "YOUR_PASS";
