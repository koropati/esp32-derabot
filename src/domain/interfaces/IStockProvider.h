#pragma once
#include "../entities/StockData.h"

// Fetches a market index snapshot from somewhere (e.g. a web API).
class IStockProvider {
public:
    virtual ~IStockProvider() = default;
    virtual bool fetch(StockData& out) = 0;  // blocking; fills out, returns success
};
