#include "ForexUseCase.h"
#include "../config/config.h"
#include <Arduino.h>

ForexUseCase::ForexUseCase(IExchangeProvider* provider) : _provider(provider) {}

bool ForexUseCase::refresh() {
    bool ok = _provider->fetch(_data);
    _lastMs = millis();   // throttle retries even on failure
    return ok;
}

bool ForexUseCase::due() const {
    return (millis() - _lastMs) >= Config::Forex::REFRESH_MS;
}
