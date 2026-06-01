#include "VoltSensor.h"
#include "../../config/config.h"
#include <Arduino.h>

bool VoltSensor::begin() {
    pinMode(Config::Pins::VOLT, INPUT);
    analogReadResolution(12);
    _ready = true;
    return true;
}

bool VoltSensor::read(SensorData& out) {
    if (!_ready) return false;
    int   raw = analogRead(Config::Pins::VOLT);
    float adc = (raw / static_cast<float>(Config::Voltage::ADC_MAX)) * Config::Voltage::VREF;
    // Un-divide to get actual battery voltage
    float ratio = (Config::Voltage::R1 + Config::Voltage::R2) / Config::Voltage::R2;
    out.voltage    = adc * ratio;
    float pct      = (out.voltage - Config::Voltage::V_MIN) /
                     (Config::Voltage::V_MAX - Config::Voltage::V_MIN) * 100.0f;
    out.batteryPct = constrain(static_cast<int>(pct), 0, 100);
    return true;
}
