#pragma once
#include "../../domain/interfaces/ISensor.h"
#include <cstdint>

// GY-271 3-axis magnetometer driver. Modules sold as "GY-271" carry either a
// QMC5883L (I2C 0x0D) or an HMC5883L (I2C 0x1E) — begin() auto-detects which.
// Computes a tilt-naive heading (0-360 deg) from the X/Y axes; keep the board
// flat for an accurate reading.
class Gy271Compass : public ISensor {
public:
    bool begin() override;
    bool read(SensorData& out) override;
    bool isReady() const override { return _ready; }

    // Live single-axis read for the dedicated compass screen (returns heading in
    // degrees, or a negative value if no reading is available).
    float readHeading();

private:
    enum class Chip { None, QMC5883L, HMC5883L };

    bool _writeReg(uint8_t reg, uint8_t val);
    bool _readRaw(int16_t& x, int16_t& y, int16_t& z);

    Chip    _chip  = Chip::None;
    uint8_t _addr  = 0;
    bool    _ready = false;
};
