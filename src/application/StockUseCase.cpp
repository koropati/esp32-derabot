#include "StockUseCase.h"
#include "../config/config.h"
#include <Arduino.h>

StockUseCase::StockUseCase(IStockProvider* provider) : _provider(provider) {}

bool StockUseCase::refresh() {
    bool ok = _provider->fetch(_data);
    _lastMs = millis();   // throttle retries even on failure
    return ok;
}

bool StockUseCase::due() const {
    return (millis() - _lastMs) >= Config::Stock::REFRESH_MS;
}
