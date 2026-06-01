#include "EspWifiManager.h"
#include <WiFi.h>

bool EspWifiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    return true;
}

bool EspWifiManager::connect(const WifiCredentials& creds) {
    WiFi.begin(creds.ssid.c_str(), creds.password.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(200);
    }
    return WiFi.status() == WL_CONNECTED;
}

void EspWifiManager::disconnect() { WiFi.disconnect(); }
bool EspWifiManager::isConnected() const { return WiFi.status() == WL_CONNECTED; }

std::vector<WifiNetwork> EspWifiManager::scan() {
    std::vector<WifiNetwork> networks;
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        WifiNetwork net;
        net.ssid    = WiFi.SSID(i);
        net.rssi    = WiFi.RSSI(i);
        net.secured = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        networks.push_back(net);
    }
    WiFi.scanDelete();
    return networks;
}

int32_t EspWifiManager::getRssi() const { return WiFi.RSSI(); }
String  EspWifiManager::getIp()   const { return WiFi.localIP().toString(); }
