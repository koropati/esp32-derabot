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
    virtual bool saveEco(bool on) = 0;             // power-save preference
    virtual bool loadEco(bool& on) = 0;
    virtual bool saveMotionWake(bool on) = 0;      // wake screen on device movement
    virtual bool loadMotionWake(bool& on) = 0;
    virtual bool saveStock(const String& symbol) = 0;  // selected market ticker
    virtual bool loadStock(String& symbol) = 0;
    virtual void clear() = 0;
};
