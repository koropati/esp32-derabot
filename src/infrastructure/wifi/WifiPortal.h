#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <functional>
#include <vector>
#include "../../domain/entities/WifiCredentials.h"

// Captive-portal web server for entering WiFi credentials from a phone/laptop.
// It runs entirely on the soft-AP side: a DNS wildcard points every lookup at
// the device so the phone auto-opens the setup page, which lists scanned
// networks and POSTs the chosen SSID + password back. The chosen credentials
// are exposed via submitted()/ssid()/password(); the caller does the actual
// save + connect so storage logic stays out of infrastructure.
class WifiPortal {
public:
    using ScanFn = std::function<std::vector<WifiNetwork>()>;

    void   begin(ScanFn scan);   // assumes the soft-AP is already up
    void   loop();               // service DNS + HTTP — call every iteration
    void   stop();

    bool   submitted() const { return _submitted; }
    String ssid()      const { return _ssid; }
    String password()  const { return _password; }
    void   clearSubmitted()  { _submitted = false; }

private:
    void   _handleRoot();
    void   _handleSave();
    void   _handleNotFound();
    String _pageHtml() const;

    WebServer     _server{80};
    DNSServer     _dns;
    ScanFn        _scanFn;
    std::vector<WifiNetwork> _nets;
    bool   _running   = false;
    bool   _submitted = false;
    String _ssid;
    String _password;
};
