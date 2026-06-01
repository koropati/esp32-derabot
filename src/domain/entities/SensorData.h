#pragma once
#include <cstdint>

struct SensorData {
    float    temperature = 0.0f;   // Celsius
    float    humidity    = 0.0f;   // %
    float    pressure    = 0.0f;   // hPa
    float    soundDb     = 0.0f;   // dB SPL
    float    voltage     = 0.0f;   // V
    int      batteryPct  = 0;      // 0-100
    int32_t  rssi        = 0;      // dBm
    int      signalPct   = 0;      // 0-100
    uint32_t timestamp   = 0;      // millis()
    bool     valid       = false;
};
