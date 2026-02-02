# LFR PID Tuner (ESP32 Dev Module) — Wiring + Code Guide

This project is a **modular Line Follower Robot (LFR)** codebase for:

- **ESP32 Dev Module (WROOM-32)**
- **TB6612FNG motor driver**
- **2× N20 motors**
- **12-channel visible-light sensor array (with internal MUX)**
- **Wi‑Fi SoftAP dashboard** for **live PID tuning** (no USB needed while running)

> Sensor polarity assumed: **black/dark → ADC HIGH**, **white/bright → ADC LOW**  
> Track: **black line on lighter floor** (3 cm line works well with the centroid + threshold approach here).

---

## 1) Power wiring (IMPORTANT — prevents resets/burn)

### Recommended (best)
- **12V LiPo → Buck #1 → 6–8V → TB6612 VM (motors)**
- **12V LiPo → Buck #2 → 5V → ESP32 VIN + Sensor VCC (if 5V board)**

### Your “one buck” setup (works if you do this carefully)
- **12V LiPo → Buck → 5V**
- Use this same **5V** for:
  - **ESP32 VIN / 5V pin**
  - **TB6612 VM** (motors)

**Must add capacitors:**
- Place **470µF–1000µF** electrolytic capacitor across **TB6612 VM ↔ GND** (close to driver)
- Place **220µF–470µF** electrolytic capacitor across **ESP32 5V ↔ GND** (close to ESP32)
- Optional: 0.1µF ceramic caps near sensor VCC

**Ground rules (mandatory):**
- Battery GND, buck GND, TB6612 GND, sensor GND, ESP32 GND → **ALL COMMON**

---

## 2) TB6612FNG wiring (ESP32 Dev Module)

### Motor driver pins → ESP32 pins
| TB6612 Pin | Function | ESP32 GPIO |
|---|---|---|
| STBY | Standby enable | **GPIO23** |
| AIN1 | Left motor dir | **GPIO18** |
| AIN2 | Left motor dir | **GPIO19** |
| PWMA | Left motor PWM | **GPIO25** |
| BIN1 | Right motor dir | **GPIO16** |
| BIN2 | Right motor dir | **GPIO17** |
| PWMB | Right motor PWM | **GPIO26** |

### TB6612 power
| TB6612 Pin | Connect to |
|---|---|
| VM | **5V from buck** (or 6–8V if you have motor buck) |
| VCC (logic) | **3.3V from ESP32** |
| GND | **Common GND** |

### Motors
| TB6612 Output | Connect to |
|---|---|
| A01 / A02 | **Left N20 motor** |
| B01 / B02 | **Right N20 motor** |

**If a motor runs backwards**
- Flip the motor wires (swap A01/A02 or B01/B02), **OR**
- Change `MOTOR_SIGN_L` / `MOTOR_SIGN_R` in `include/config.h` to `-1`.

---

## 3) 12‑channel MUX sensor wiring (ESP32 Dev Module)

### Select pins
| Sensor Pin | ESP32 GPIO |
|---|---|
| SEL_A | **GPIO32** |
| SEL_B | **GPIO33** |

### Analog inputs (ADC1, Wi‑Fi safe)
| Sensor Pin | ESP32 GPIO |
|---|---|
| IN_A | **GPIO36** |
| IN_B | **GPIO39** |
| IN_C | **GPIO34** |
| IN_D | **GPIO35** |

> These are **ADC1** pins, so Wi‑Fi + ADC works together (ADC2 often conflicts with Wi‑Fi).

### Sensor power notes
- If your sensor board works on **3.3V**, power it from ESP32 3V3.
- If your sensor board only lights/works on **5V**, you can power it from **5V**, BUT:
  - Ensure its analog outputs going into ESP32 ADC never exceed **3.3V**
  - If unsure, use a **voltage divider** or buffer.

---

## 4) Software overview

### Folder structure
```
lfr_pid_tuner/
  include/
    config.h
    motor_tb6612.h
    sensor_mux12.h
    pid_controller.h
    wifi_tuner.h
  src/
    motor_tb6612.cpp
    sensor_mux12.cpp
    pid_controller.cpp
    wifi_tuner.cpp
    main.cpp
  platformio.ini
```

### Key settings (edit in `include/config.h`)
- **Pins** (TB6612 + sensor mux)
- **BASE_SPEED / MAX_SPEED / MIN_MOVE**
- **Sensor thresholds**: `CONTRAST_TH`, `TH_FRAC`, `LINE_SUM_TH`
- **PID defaults**: `KP_DEFAULT`, `KI_DEFAULT`, `KD_DEFAULT`
- **Wi‑Fi**: `WIFI_AP_SSID`, `WIFI_AP_PASS`
- **PID tuning step sizes**: `KP_STEP`, `KI_STEP`, `KD_STEP`

---

## 5) How the robot runs (state machine)

### 1) Auto calibration (boot)
- Robot spins for `CALIB_MS`
- Each sensor stores **min/max**
- Then switches to **RUN**

### 2) Run mode
- Reads 12 sensors via mux
- Normalizes each channel to **0..1000**
- Applies EMA filter (`SENSOR_ALPHA`)
- Detects line position using:
  - **contrast check** (reject uniform dark/bright)
  - **adaptive threshold**
  - **centroid / weighted average**
- PID computes correction:
  - `error = pos - 5500`
  - `left = base + corr`, `right = base - corr`
  - base auto‑slows on sharp turns (`TURN_SLOW`)
- If line lost:
  - short seek turn based on last error direction
  - after `LOST_STOP_COUNT` cycles → stop

---

## 6) Wi‑Fi PID Tuning (no laptop needed)

### Connect
1. Power on robot
2. On your phone, connect to Wi‑Fi: **LFR-Tuner**
3. Password: **12345678**
4. Open browser: **http://192.168.4.1**

### What you can do
- Increase/decrease **Kp / Ki / Kd**
- Recalibrate / Stop / Run
- See live status: `pos, sum, err, lost, motor outputs`

> Note: `KP_STEP = 0.5` is a big step.  
> For easier tuning, set **KP_STEP = 0.05** in `include/config.h`.

---

## 7) Build & upload (PlatformIO)

1. Open project in VS Code + PlatformIO
2. Select environment: `esp32dev`
3. Build/Upload
4. Monitor (optional): **115200 baud**

---

## 8) Quick troubleshooting

### Motor problems
- **One motor doesn’t move**:
  - Check A01/A02 (left) and B01/B02 (right) wiring
  - Confirm STBY is HIGH
  - Confirm VM is stable (add capacitors!)
- **Robot steers wrong way**:
  - Flip motor wires or set `MOTOR_SIGN_L/R = -1`

### Sensor problems
- **Always reads black**:
  - Too high from ground (shadow)
  - Wrong VCC voltage
  - `CONTRAST_TH` too low (increase to reject uniform readings)
- **Loses line often**:
  - Decrease speed (`BASE_SPEED`)
  - Increase `TURN_SLOW`
  - Adjust `TH_FRAC` (try 0.60) and `LINE_SUM_TH`

### ESP32 resets/heats
- Add the capacitors
- Improve grounding
- Use separate buck for motors if possible

---

## 9) Pin reference (matches code defaults)

These are the pins used in `include/config.h`:

- **Motors**: STBY=23, AIN1=18, AIN2=19, PWMA=25, BIN1=16, BIN2=17, PWMB=26  
- **Sensor mux**: SEL_A=32, SEL_B=33, IN_A=36, IN_B=39, IN_C=34, IN_D=35

If you change wiring, update `include/config.h`.

---

### License / Usage
Use freely for your robot project. Customize thresholds and PID on your track for best performance.
