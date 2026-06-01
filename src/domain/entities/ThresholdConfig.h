#pragma once

struct ThresholdConfig {
    float tempMax  = 40.0f;   // Celsius
    float tempMin  = 0.0f;    // Celsius
    float humidMax = 85.0f;   // %
    float pressMax = 1050.0f; // hPa
    float soundMax = 80.0f;   // dB
    float voltMin  = 0.0f;    // V (0.0 = disabled, set from Settings screen)
    bool  enabled  = false;  // Off by default — enable from Settings screen
};
