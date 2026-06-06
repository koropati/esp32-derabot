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

void EspWifiManager::setPowerSave(bool on) {
    // Modem-sleep: the radio parks between AP beacons. Keeps the connection (and
    // MQTT) alive while cutting average current substantially.
    WiFi.setSleep(on ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE);
}

bool EspWifiManager::startAP(const String& ssid, const String& pass) {
    // AP_STA keeps the station side alive so we can still scan and connect to
    // the network the user picks while the config hotspot is up.
    WiFi.mode(WIFI_AP_STA);
    bool ok = pass.isEmpty()
        ? WiFi.softAP(ssid.c_str())
        : WiFi.softAP(ssid.c_str(), pass.c_str());
    Serial.printf("[WiFi] SoftAP '%s' %s, IP=%s\n",
                  ssid.c_str(), ok ? "up" : "FAILED", WiFi.softAPIP().toString().c_str());
    return ok;
}

void EspWifiManager::stopAP() {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
}

String EspWifiManager::getApIp()   const { return WiFi.softAPIP().toString(); }
int    EspWifiManager::apClients() const { return WiFi.softAPgetStationNum(); }
