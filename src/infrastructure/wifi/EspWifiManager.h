#pragma once
#include "../../domain/interfaces/IWifiManager.h"

class EspWifiManager : public IWifiManager {
public:
    bool begin()    override;
    bool connect(const WifiCredentials& creds) override;
    void disconnect() override;
    bool isConnected() const override;
    std::vector<WifiNetwork> scan() override;
    int32_t getRssi() const override;
    String  getIp()   const override;
};
