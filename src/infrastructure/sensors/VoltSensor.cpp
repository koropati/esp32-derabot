#include "VoltSensor.h"
#include "../../config/config.h"
#include <Arduino.h>

bool VoltSensor::begin() {
    pinMode(Config::Pins::VOLT, INPUT);
    analogReadResolution(12);
    // 11 dB attenuation -> usable input range up to ~2.5-3.1 V at the pin,
    // which fits a 1S LiPo (max 4.2 V) through the /2 divider (~2.1 V).
    analogSetPinAttenuation(Config::Pins::VOLT, ADC_11db);
    _ready = true;
    return true;
}

bool VoltSensor::read(SensorData& out) {
    if (!_ready) return false;

    // Average several samples using the per-chip factory calibration
    // (analogReadMilliVolts) — far more accurate and stable than raw*VREF/4095.
    uint32_t sumMv = 0;
    for (int i = 0; i < Config::Voltage::SAMPLES; i++)
        sumMv += analogReadMilliVolts(Config::Pins::VOLT);
    float vPin = (sumMv / static_cast<float>(Config::Voltage::SAMPLES)) / 1000.0f;

    // Un-divide to recover battery voltage, then apply calibration factor.
    // batteryPct is derived later (in the main loop) from the user-configurable
    // gauge range, so here we only report the measured voltage.
    float ratio = (Config::Voltage::R1 + Config::Voltage::R2) / Config::Voltage::R2;
    out.voltage = vPin * ratio * Config::Voltage::CAL;

    if (Config::Voltage::DEBUG)
        Serial.printf("[VOLT] pin=%.3fV  battery=%.3fV\n", vPin, out.voltage);
    return true;
}
