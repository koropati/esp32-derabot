#include "SensorUseCase.h"
#include <Arduino.h>

SensorUseCase::SensorUseCase(std::vector<ISensor*> sensors)
    : _sensors(std::move(sensors)) {}

bool SensorUseCase::begin() {
    bool ok = true;
    for (auto* s : _sensors) ok &= s->begin();
    return ok;
}

SensorData SensorUseCase::read() {
    SensorData data = _last;
    for (auto* s : _sensors) {
        if (s->isReady()) s->read(data);
    }
    data.valid     = true;
    data.timestamp = millis();
    _last = data;
    return data;
}

void SensorUseCase::updateWifiInfo(int32_t rssi, int signalPct) {
    _last.rssi      = rssi;
    _last.signalPct = signalPct;
}
