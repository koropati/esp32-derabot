#pragma once
#include "../../domain/interfaces/ISensor.h"
#include <Adafruit_BME280.h>

class Bme280Sensor : public ISensor {
public:
    bool begin()   override;
    bool read(SensorData& out) override;
    bool isReady() const override { return _ready; }

private:
    Adafruit_BME280 _bme;
    bool            _ready = false;
};
