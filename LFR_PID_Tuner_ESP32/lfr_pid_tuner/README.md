# LFR PID Tuner (ESP32 Dev Module)

## What this project does
- Reads a 12-channel visible-light sensor array via mux.
- Auto-calibrates on boot (spins) then runs PID line following.
- Hosts a Wi-Fi SoftAP + web dashboard to tune Kp/Ki/Kd live.

## Connect
1. Power the robot.
2. On your phone, connect to Wi-Fi: **LFR-Tuner** (password: 12345678)
3. Open: **http://192.168.4.1**
4. Use +/- buttons to tune PID and Recalibrate/Stop/Run.

## Notes
- Sensor polarity is set for: black/dark -> ADC HIGH.
- If robot steers the wrong way, flip MOTOR_SIGN_L or MOTOR_SIGN_R in include/config.h
- If line detection is unreliable:
  - Increase CONTRAST_TH to reject uniform readings
  - Decrease TH_FRAC if it loses the line too often
  - Adjust LINE_SUM_TH based on your track reflectance
