#pragma once
#include "../../domain/interfaces/IStockProvider.h"

// Pulls intraday data for the given ticker from Yahoo Finance's chart JSON
// endpoint over HTTPS and parses only the fields we need with a streaming filter.
class YahooStockClient : public IStockProvider {
public:
    bool fetch(const String& symbol, StockData& out) override;
};
