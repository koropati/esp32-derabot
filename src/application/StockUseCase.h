#pragma once
#include "../domain/interfaces/IStockProvider.h"
#include "../domain/interfaces/IStorage.h"
#include "../domain/entities/StockData.h"

// Tracks which market code is shown (chosen from Config::Stock::LIST, persisted
// in storage) and fetches its snapshot on demand.
class StockUseCase {
public:
    StockUseCase(IStockProvider* provider, IStorage* storage);
    void begin();                         // restore the saved symbol selection
    bool refresh();                       // blocking fetch, updates cached data
    bool due() const;                     // true when REFRESH_MS has elapsed
    const StockData& data() const { return _data; }

    int         symbolIndex() const { return _idx; }
    const char* label()       const;      // short on-screen name of current code
    const char* symbol()      const;      // Yahoo ticker of current code
    void        setSymbolIndex(int idx);  // clamp, persist, force a re-fetch

private:
    IStockProvider* _provider;
    IStorage*       _storage;
    StockData       _data;
    uint32_t        _lastMs = 0;
    int             _idx    = 0;          // index into Config::Stock::LIST
};
