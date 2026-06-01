#pragma once
#include "../entities/WifiCredentials.h"
#include "../entities/ThresholdConfig.h"

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual bool begin() = 0;
    virtual bool saveWifi(const WifiCredentials& creds) = 0;
    virtual bool loadWifi(WifiCredentials& creds) = 0;
    virtual bool saveThreshold(const ThresholdConfig& cfg) = 0;
    virtual bool loadThreshold(ThresholdConfig& cfg) = 0;
    virtual void clear() = 0;
};
