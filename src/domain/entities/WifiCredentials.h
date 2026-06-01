#pragma once
#include <Arduino.h>

struct WifiCredentials {
    String ssid;
    String password;
};

struct WifiNetwork {
    String  ssid;
    int32_t rssi    = 0;
    bool    secured = false;
};
