#pragma once
#include "../domain/interfaces/ISensor.h"
#include "../domain/entities/SensorData.h"
#include <vector>

class SensorUseCase {
public:
    explicit SensorUseCase(std::vector<ISensor*> sensors);
    bool begin();
    SensorData read();
    const SensorData& lastData() const { return _last; }
    void updateWifiInfo(int32_t rssi, int signalPct);
    void setBatteryPct(int pct) { _last.batteryPct = pct; }

private:
    std::vector<ISensor*> _sensors;
    SensorData            _last;
};
