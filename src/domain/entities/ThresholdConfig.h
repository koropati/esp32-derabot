#pragma once

struct ThresholdConfig {
    float tempMax  = 40.0f;   // Celsius
    float tempMin  = 0.0f;    // Celsius
    float humidMax = 85.0f;   // %
    float pressMax = 1050.0f; // hPa
    float soundMax = 80.0f;   // dB
    float voltMin  = 0.0f;    // V (0.0 = disabled, set from Settings screen)
    bool  enabled  = false;  // Off by default — enable from Settings screen

    // Master buzzer volume as a percentage (0-100). Scales the PWM duty of every
    // sound — button clicks, alarm, and all jingles. 100% = the original loudness,
    // 0% = muted. Adjustable from the Settings screen.
    float volume   = 40.0f;

    // Battery gauge calibration — maps measured voltage to the 0-100% indicator.
    // Adjust from Settings to match a different cell's empty/full points.
    float battMin  = 3.2f;    // V read as 0%  (battery empty)
    float battMax  = 3.7f;    // V read as 100% (battery full)
};
