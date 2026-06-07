#include "StockUseCase.h"
#include "../config/config.h"
#include <Arduino.h>

StockUseCase::StockUseCase(IStockProvider* provider, IStorage* storage)
    : _provider(provider), _storage(storage) {}

void StockUseCase::begin() {
    // Restore the saved ticker and map it back to a list index. Falls back to the
    // first entry (the default) if nothing is saved or the code is no longer known.
    String saved;
    if (_storage && _storage->loadStock(saved)) {
        for (int i = 0; i < Config::Stock::LIST_COUNT; i++) {
            if (saved == Config::Stock::LIST[i].symbol) { _idx = i; break; }
        }
    }
}

const char* StockUseCase::label()  const { return Config::Stock::LIST[_idx].label;  }
const char* StockUseCase::symbol() const { return Config::Stock::LIST[_idx].symbol; }

void StockUseCase::setSymbolIndex(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= Config::Stock::LIST_COUNT) idx = Config::Stock::LIST_COUNT - 1;
    if (idx == _idx) return;
    _idx = idx;
    if (_storage) _storage->saveStock(Config::Stock::LIST[_idx].symbol);
    _data = StockData{};   // drop stale data so the screen reloads the new code
    _lastMs = 0;           // due() fires immediately
}

bool StockUseCase::refresh() {
    bool ok = _provider->fetch(symbol(), _data);
    _data.symbol = label();   // show the friendly name regardless of fetch result
    _lastMs = millis();       // throttle retries even on failure
    return ok;
}

bool StockUseCase::due() const {
    return (millis() - _lastMs) >= Config::Stock::REFRESH_MS;
}
