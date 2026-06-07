#pragma once
#include "../../domain/interfaces/IExchangeProvider.h"

// Pulls rupiah exchange rates from exchangerate-api's free open endpoint
// (open.er-api.com, no API key) over HTTPS and parses only the rates we need
// with a streaming filter. Base currency is USD; each configured code is
// converted to IDR as rates[IDR] / rates[code].
class ErApiClient : public IExchangeProvider {
public:
    bool fetch(ExchangeData& out) override;
};
