#include "WifiPortalScreen.h"
#include "../../config/config.h"
#include "../LoadingWindow.h"
#include <utility>

WifiPortalScreen::WifiPortalScreen(WifiUseCase* wifi, BuzzerUseCase* buzzer, PowerUseCase* power)
    : _wifi(wifi), _buzzer(buzzer), _power(power) {}

void WifiPortalScreen::enter() {
    // Defer the blocking AP bring-up (startAP + WiFi scan) to the first update()
    // so a loading window can be drawn on screen during it.
    _state     = St::Starting;
    _done      = false;
    _attempted = false;
    _failed    = false;
    if (_buzzer) _buzzer->stopClick();      // kill the menu click before blocking
    // Suspend eco for the whole session: low CPU + screen-off make the soft-AP
    // drop the phone repeatedly. Restored in exit().
    if (_power) _power->holdAwake(true);
}

void WifiPortalScreen::exit() {
    _portal.stop();
    _wifi->stopAP();
    if (_power) _power->holdAwake(false);    // resume normal eco behavior
}

void WifiPortalScreen::tick() {
    if (_state != St::Serving) return;
    _portal.loop();
    if (_portal.submitted()) {
        _targetSsid = _portal.ssid();
        _targetPass = _portal.password();
        _portal.clearSubmitted();
        _state     = St::Connecting;
        _attempted = false;
    }
}

void WifiPortalScreen::update(IDisplay& d) {
    // Bring the soft-AP + web portal up here (not in enter()) so the loading
    // window stays on screen during the blocking AP start + WiFi scan.
    if (_state == St::Starting) {
        // Scan WHILE still in pure STA mode — this is far more reliable than
        // scanning after the soft-AP is up. The list is then handed to the
        // portal so it can render without re-scanning under AP_STA.
        drawLoadingWindow(d, "Setup WiFi via HP", "Memindai WiFi...");
        d.flush();
        auto nets = _wifi->scan();

        drawLoadingWindow(d, "Setup WiFi via HP", "Mengaktifkan AP...");
        d.flush();
        _wifi->startAP();
        _portal.begin(std::move(nets), [w = _wifi]() { return w->scan(); });
        if (_buzzer) _buzzer->playWifiTune();  // chime after; tick() advances it
        _state = St::Serving;
        return;
    }

    // Perform the (blocking) connect here, after drawing a loading window.
    if (_state == St::Connecting && !_attempted) {
        drawLoadingWindow(d, "Menghubungkan", _targetSsid.substring(0, 18));
        d.flush();

        _attempted = true;
        bool ok = _wifi->connect(_targetSsid, _targetPass);  // saves on success
        if (ok) {
            _portal.stop();
            _wifi->stopAP();
            _state = St::Success;
        } else {
            _failed = true;
            _state  = St::Serving;  // keep the portal up so the user can retry
        }
        return;
    }

    if (_state == St::Success) {
        d.clear();
        d.drawText(0, 0, "WiFi Tersambung!");
        d.drawLine(0, 11, d.width() - 1, 11);
        d.drawText(0, 20, _targetSsid.substring(0, 21));
        d.drawText(0, 32, ("IP:" + _wifi->getIp()).substring(0, 21));
        d.drawLine(0, 55, d.width() - 1, 55);
        d.drawText(0, 57, "Ctr: Selesai");
        d.flush();
        return;
    }

    // Serving — show how to join and configure (SSID, password, then the URL)
    d.clear();
    d.drawText(0, 0, "Setup WiFi via HP");
    d.drawLine(0, 11, d.width() - 1, 11);
    d.drawText(0, 14, (String("WiFi: ") + Config::Ap::SSID).substring(0, 21).c_str());
    bool hasPass = Config::Ap::PASS[0] != '\0';
    d.drawText(0, 24, (String("Pass: ") + (hasPass ? Config::Ap::PASS : "(terbuka)")).substring(0, 21).c_str());
    d.drawText(0, 34, "Buka browser ke:");
    d.drawText(0, 44, (String("  ") + _wifi->apIp()).c_str());
    d.drawLine(0, 55, d.width() - 1, 55);
    char foot[24];
    if (_failed)
        snprintf(foot, sizeof(foot), "Gagal-ulangi  Klien:%d", _wifi->apClients());
    else
        snprintf(foot, sizeof(foot), "<Kembali     Klien:%d", _wifi->apClients());
    d.drawText(0, 57, foot);
    d.flush();
}

void WifiPortalScreen::onButton(ButtonEvent evt) {
    if (_state == St::Success) {
        if (evt == ButtonEvent::Center || evt == ButtonEvent::Left) _done = true;
        return;
    }
    if (evt == ButtonEvent::Left) _done = true;  // exit the portal
}
