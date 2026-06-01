#pragma once
#include "../entities/SensorData.h"

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual bool begin() = 0;
    virtual bool read(SensorData& out) = 0;
    virtual bool isReady() const = 0;
};
