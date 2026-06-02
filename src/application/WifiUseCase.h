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

    // Config-portal access point (credentials entered from a phone/laptop)
    bool   startAP();
    void   stopAP();
    String apIp()      const;
    int    apClients() const;

private:
    IWifiManager*   _wifi;
    IStorage*       _storage;
    WifiCredentials _saved;
};
