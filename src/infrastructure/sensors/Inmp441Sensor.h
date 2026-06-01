#pragma once
#include "../../domain/interfaces/ISensor.h"
#include <driver/i2s.h>

class Inmp441Sensor : public ISensor {
public:
    bool begin()   override;
    bool read(SensorData& out) override;
    bool isReady() const override { return _ready; }

private:
    bool  _ready = false;
    float computeDb(const int32_t* buf, size_t count);
};
