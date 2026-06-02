#pragma once
#include "../../domain/interfaces/IStockProvider.h"

// Pulls IHSG (^JKSE) intraday data from Yahoo Finance's chart JSON endpoint over
// HTTPS and parses only the fields we need with a streaming filter.
class YahooStockClient : public IStockProvider {
public:
    bool fetch(StockData& out) override;
};
