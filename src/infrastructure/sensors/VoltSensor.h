#pragma once
#include "../../domain/interfaces/ISensor.h"

class VoltSensor : public ISensor {
public:
    bool begin()   override;
    bool read(SensorData& out) override;
    bool isReady() const override { return _ready; }

private:
    bool _ready = false;
};
