#pragma once
#include <Arduino.h>
#include <vector>

// A single market index snapshot plus its intraday curve (for the graph).
struct StockData {
    String   symbol    = "IHSG";
    float    price     = 0.0f;   // latest index value
    float    prevClose = 0.0f;   // previous session close (graph baseline)
    float    change    = 0.0f;   // price - prevClose
    float    changePct = 0.0f;   // percentage change
    std::vector<float> points;   // intraday closes, oldest -> newest
    bool     valid     = false;
    uint32_t fetchedAt = 0;      // millis() of last successful fetch
    String   error;              // human-readable reason when !valid
};
