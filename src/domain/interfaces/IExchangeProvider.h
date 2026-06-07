#pragma once
#include "../entities/ExchangeData.h"

// Fetches current rupiah exchange rates from somewhere (e.g. a web API).
class IExchangeProvider {
public:
    virtual ~IExchangeProvider() = default;
    virtual bool fetch(ExchangeData& out) = 0;  // blocking; fills out, returns success
};
