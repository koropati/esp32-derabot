#include "WifiScanScreen.h"
#include <WiFi.h>

WifiScanScreen::WifiScanScreen(WifiUseCase* wifi) : _wifi(wifi) {}

void WifiScanScreen::enter() {
    _done        = false;
    _wantConnect = false;
    _cursor      = 0;
    _selectedSsid = "";
    _networks.clear();
    _scanning = true;
    // Start async scan — non-blocking, returns immediately
    WiFi.scanNetworks(/*async=*/true);
}

void WifiScanScreen::update(IDisplay& d) {
    // Poll async scan result
    if (_scanning) {
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            // Still scanning — show progress
            d.clear();
            d.drawText(0, 0, "WiFi Networks");
            d.drawLine(0, 9, d.width() - 1, 9);
            d.drawText(10, 28, "Mencari jaringan");
            d.drawText(30, 42, "silahkan tunggu");
            d.drawLine(0, 55, d.width() - 1, 55);
            d.drawText(0, 57, "< Batal");
            d.flush();
            return;
        }
        // Scan done (n >= 0) or failed (n == WIFI_SCAN_FAILED)
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                WifiNetwork net;
                net.ssid    = WiFi.SSID(i);
                net.rssi    = WiFi.RSSI(i);
                net.secured = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                _networks.push_back(net);
            }
            WiFi.scanDelete();
        }
        _scanning = false;
        _cursor   = 0;
    }

    // Render network list
    d.clear();
    d.drawText(0, 0, "WiFi Networks");
    d.drawLine(0, 9, d.width() - 1, 9);

    if (_networks.empty()) {
        d.drawText(0, 20, "Tidak ada jaringan.");
        d.drawText(0, 32, "Rgt: Scan ulang");
        d.drawLine(0, 55, d.width() - 1, 55);
        d.drawText(0, 57, "< Kembali  Ulang:>");
    } else {
        constexpr int VISIBLE = 4;
        int start = max(0, _cursor - VISIBLE + 1);
        for (int i = 0; i < VISIBLE; i++) {
            int idx = start + i;
            if (idx >= (int)_networks.size()) break;
            String line;
            line  = (idx == _cursor) ? ">" : " ";
            line += _networks[idx].secured ? "*" : " ";
            String ssid = _networks[idx].ssid;
            if (ssid.length() > 17) ssid = ssid.substring(0, 16) + "~";
            line += ssid;
            d.drawText(0, 11 + i * 11, line);
        }
        char info[12];
        snprintf(info, sizeof(info), "%d/%d", _cursor + 1, (int)_networks.size());
        d.drawText(d.width() - 6 * (int)strlen(info) - 1, 0, info);
        d.drawLine(0, 55, d.width() - 1, 55);
        d.drawText(0, 57, "< Atas    Ctr:Pilih >");
    }
    d.flush();
}

void WifiScanScreen::onButton(ButtonEvent evt) {
    // While scanning: only Left cancels
    if (_scanning) {
        if (evt == ButtonEvent::Left) { WiFi.scanDelete(); _done = true; }
        return;
    }

    switch (evt) {
        case ButtonEvent::Left:
            if (_networks.empty() || _cursor == 0) {
                _done = true;  // back to menu from top of list
            } else {
                _cursor--;
            }
            break;
        case ButtonEvent::Right:
            if (_networks.empty()) {
                _networks.clear();
                _scanning = true;
                WiFi.scanNetworks(true);
            } else {
                _cursor = min((int)_networks.size() - 1, _cursor + 1);
            }
            break;
        case ButtonEvent::Center:
            if (!_networks.empty()) {
                _selectedSsid = _networks[_cursor].ssid;
                _wantConnect  = true;
                _done         = true;
            }
            break;
        default:
            break;
    }
}
