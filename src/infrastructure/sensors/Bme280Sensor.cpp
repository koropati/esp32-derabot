#include "Bme280Sensor.h"
#include "../../config/config.h"
#include <Wire.h>
#include <Arduino.h>

static uint8_t readChipId(uint8_t addr) {
    Wire.beginTransmission(addr);
    Wire.write(0xD0);
    Wire.endTransmission(false);
    Wire.requestFrom(addr, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0x00;
}

bool Bme280Sensor::begin() {
    Wire.begin(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL);
    delay(10);

    uint8_t chipId = readChipId(Config::I2cAddr::BME280);
    Serial.printf("[BME280] chip ID=0x%02X at 0x%02X\n", chipId, Config::I2cAddr::BME280);

    // Try primary address with retries
    for (int i = 0; i < 3 && !_ready; i++) {
        _ready = _bme.begin(Config::I2cAddr::BME280, &Wire);
        if (!_ready) delay(50);
    }

    // Some clones sit at 0x77 — try fallback
    if (!_ready) {
        Serial.println("[BME280] retrying at 0x77...");
        _ready = _bme.begin(0x77, &Wire);
    }

    if (_ready) {
        Serial.printf("[BME280] OK (sensor ID=0x%02X)\n", _bme.sensorID());
    } else {
        Serial.println("[BME280] init failed - check address/wiring");
    }
    return _ready;
}

bool Bme280Sensor::read(SensorData& out) {
    if (!_ready) return false;
    out.temperature = _bme.readTemperature();
    out.humidity    = _bme.readHumidity();
    out.pressure    = _bme.readPressure() / 100.0f;
    if (isnan(out.temperature)) {
        // Sensor glitched — re-init BME280 only, do NOT touch Wire (shared with OLED)
        _ready = _bme.begin(Config::I2cAddr::BME280, &Wire);
        return false;
    }
    return true;
}
