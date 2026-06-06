#include "Gy271Compass.h"
#include "../../config/config.h"
#include <Wire.h>
#include <Arduino.h>
#include <math.h>

static bool i2cPresent(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

bool Gy271Compass::_writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

bool Gy271Compass::begin() {
    // I2C bus already started by BME280/OLED; begin() again is harmless and keeps
    // this sensor self-contained.
    Wire.begin(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL);
    delay(10);

    if (i2cPresent(Config::I2cAddr::QMC5883)) {
        _chip = Chip::QMC5883L;
        _addr = Config::I2cAddr::QMC5883;
        _writeReg(0x0A, 0x80);   // soft reset
        delay(10);
        _writeReg(0x0B, 0x01);   // SET/RESET period (datasheet-recommended 0x01)
        _writeReg(0x09, 0x1D);   // OSR=512, RNG=8G, ODR=200Hz, MODE=continuous
        _ready = true;
    } else if (i2cPresent(Config::I2cAddr::HMC5883)) {
        _chip = Chip::HMC5883L;
        _addr = Config::I2cAddr::HMC5883;
        _writeReg(0x00, 0x70);   // Config A: 8 samples averaged, 15 Hz
        _writeReg(0x01, 0xA0);   // Config B: gain
        _writeReg(0x02, 0x00);   // Mode: continuous measurement
        _ready = true;
    } else {
        _chip  = Chip::None;
        _ready = false;
    }

    Serial.printf("[GY271] %s\n",
        _chip == Chip::QMC5883L ? "QMC5883L @0x0D OK" :
        _chip == Chip::HMC5883L ? "HMC5883L @0x1E OK" :
                                  "not found - check wiring");
    return _ready;
}

bool Gy271Compass::_readRaw(int16_t& x, int16_t& y, int16_t& z) {
    // Read bytes into ordered locals — `a | (b << 8)` would otherwise rely on
    // unspecified Wire.read() evaluation order.
    if (_chip == Chip::QMC5883L) {
        Wire.beginTransmission(_addr);
        Wire.write(0x00);                          // data registers start at 0x00
        if (Wire.endTransmission(false) != 0) return false;
        if (Wire.requestFrom(_addr, (uint8_t)6) != 6) return false;
        uint8_t xl = Wire.read(), xh = Wire.read();   // little-endian
        uint8_t yl = Wire.read(), yh = Wire.read();
        uint8_t zl = Wire.read(), zh = Wire.read();
        x = (int16_t)(xl | (xh << 8));
        y = (int16_t)(yl | (yh << 8));
        z = (int16_t)(zl | (zh << 8));
        return true;
    }
    if (_chip == Chip::HMC5883L) {
        Wire.beginTransmission(_addr);
        Wire.write(0x03);                          // data registers start at 0x03
        if (Wire.endTransmission(false) != 0) return false;
        if (Wire.requestFrom(_addr, (uint8_t)6) != 6) return false;
        uint8_t xh = Wire.read(), xl = Wire.read();   // big-endian, order X, Z, Y
        uint8_t zh = Wire.read(), zl = Wire.read();
        uint8_t yh = Wire.read(), yl = Wire.read();
        x = (int16_t)((xh << 8) | xl);
        z = (int16_t)((zh << 8) | zl);
        y = (int16_t)((yh << 8) | yl);
        return true;
    }
    return false;
}

float Gy271Compass::readHeading() {
    if (!_ready) return -1.0f;
    int16_t x, y, z;
    if (!_readRaw(x, y, z)) return -1.0f;

    float heading = atan2f((float)y, (float)x) * 180.0f / (float)PI;
    heading += Config::Compass::DECLINATION;
    if (heading < 0.0f)     heading += 360.0f;
    if (heading >= 360.0f)  heading -= 360.0f;
    return heading;
}

bool Gy271Compass::read(SensorData& out) {
    if (!_ready) { out.compassOk = false; return false; }
    float h = readHeading();
    if (h < 0.0f) { out.compassOk = false; return false; }
    out.heading   = h;
    out.compassOk = true;
    return true;
}
