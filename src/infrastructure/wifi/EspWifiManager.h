#pragma once
#include "../../domain/interfaces/IWifiManager.h"
#include <esp_wifi_types.h>

class EspWifiManager : public IWifiManager {
public:
    bool begin()    override;
    bool connect(const WifiCredentials& creds) override;
    void disconnect() override;
    bool isConnected() const override;
    std::vector<WifiNetwork> scan() override;
    int32_t getRssi() const override;
    String  getIp()   const override;
    void    setPowerSave(bool on) override;

    bool    startAP(const String& ssid, const String& pass) override;
    void    stopAP() override;
    String  getApIp()   const override;
    int     apClients() const override;

private:
    // Sleep mode in effect before the soft-AP came up. Modem-sleep starves the
    // AP beacons and makes the phone drop in/out, so we force it off while the
    // config hotspot is live and restore the saved mode in stopAP().
    wifi_ps_type_t _savedPs = WIFI_PS_NONE;
};
