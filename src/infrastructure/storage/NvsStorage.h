#pragma once
#include "../../domain/interfaces/IStorage.h"
#include <Preferences.h>

class NvsStorage : public IStorage {
public:
    bool begin() override;
    bool saveWifi(const WifiCredentials& creds) override;
    bool loadWifi(WifiCredentials& creds) override;
    bool saveThreshold(const ThresholdConfig& cfg) override;
    bool loadThreshold(ThresholdConfig& cfg) override;
    bool saveEco(bool on) override;
    bool loadEco(bool& on) override;
    bool saveMotionWake(bool on) override;
    bool loadMotionWake(bool& on) override;
    bool saveStock(const String& symbol) override;
    bool loadStock(String& symbol) override;
    void clear() override;

private:
    Preferences _prefs;
};
