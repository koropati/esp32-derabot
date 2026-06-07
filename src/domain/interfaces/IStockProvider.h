#pragma once
#include "../entities/StockData.h"

// Fetches a market index snapshot from somewhere (e.g. a web API).
class IStockProvider {
public:
    virtual ~IStockProvider() = default;
    // `symbol` is the provider-specific ticker (e.g. "^JKSE", "BBCA.JK").
    // Blocking; fills out, returns success.
    virtual bool fetch(const String& symbol, StockData& out) = 0;
};
