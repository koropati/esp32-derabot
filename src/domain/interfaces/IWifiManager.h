#pragma once
#include <Arduino.h>
#include <vector>
#include "../entities/WifiCredentials.h"

class IWifiManager {
public:
    virtual ~IWifiManager() = default;
    virtual bool begin() = 0;
    virtual bool connect(const WifiCredentials& creds) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual std::vector<WifiNetwork> scan() = 0;
    virtual int32_t getRssi() const = 0;
    virtual String  getIp()   const = 0;

    // Soft-AP (config portal) support
    virtual bool    startAP(const String& ssid, const String& pass) = 0;
    virtual void    stopAP() = 0;
    virtual String  getApIp()    const = 0;
    virtual int     apClients()  const = 0;
};
