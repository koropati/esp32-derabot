#pragma once
#include "../domain/interfaces/IExchangeProvider.h"
#include "../domain/entities/ExchangeData.h"

// Fetches and caches the current rupiah exchange rates on a refresh timer.
class ForexUseCase {
public:
    explicit ForexUseCase(IExchangeProvider* provider);
    bool refresh();                       // blocking fetch, updates cached data
    bool due() const;                     // true when REFRESH_MS has elapsed
    const ExchangeData& data() const { return _data; }

private:
    IExchangeProvider* _provider;
    ExchangeData       _data;
    uint32_t           _lastMs = 0;
};
