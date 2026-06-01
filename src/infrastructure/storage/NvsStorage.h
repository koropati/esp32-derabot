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
    void clear() override;

private:
    Preferences _prefs;
};
