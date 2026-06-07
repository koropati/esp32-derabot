#pragma once
#include <Arduino.h>
#include <vector>

// One currency's value expressed in rupiah (1 unit of `code` = `idr` IDR).
struct ExchangeRate {
    String code;
    float  idr = 0.0f;
};

// A snapshot of rupiah exchange rates for the configured currencies.
struct ExchangeData {
    std::vector<ExchangeRate> rates;
    String   updated;            // source's last-update date (short, human text)
    bool     valid     = false;
    uint32_t fetchedAt = 0;      // millis() of last successful fetch
    String   error;              // human-readable reason when !valid
};
