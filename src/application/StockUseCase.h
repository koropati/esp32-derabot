#pragma once
#include "../domain/interfaces/IStockProvider.h"
#include "../domain/entities/StockData.h"

class StockUseCase {
public:
    explicit StockUseCase(IStockProvider* provider);
    bool refresh();                       // blocking fetch, updates cached data
    bool due() const;                     // true when REFRESH_MS has elapsed
    const StockData& data() const { return _data; }

private:
    IStockProvider* _provider;
    StockData       _data;
    uint32_t        _lastMs = 0;
};
