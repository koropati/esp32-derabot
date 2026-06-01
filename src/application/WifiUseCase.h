#pragma once
#include "../domain/interfaces/IWifiManager.h"
#include "../domain/interfaces/IStorage.h"
#include <vector>

class WifiUseCase {
public:
    WifiUseCase(IWifiManager* wifi, IStorage* storage);
    bool begin();
    bool autoConnect();
    bool connect(const String& ssid, const String& password);
    void disconnect();
    bool isConnected() const;
    std::vector<WifiNetwork> scan();
    int32_t getRssi() const;
    String  getIp()   const;
    WifiCredentials getSaved() const { return _saved; }

private:
    IWifiManager*   _wifi;
    IStorage*       _storage;
    WifiCredentials _saved;
};
