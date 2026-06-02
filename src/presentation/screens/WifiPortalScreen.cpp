#include "WifiPortalScreen.h"
#include "../../config/config.h"

WifiPortalScreen::WifiPortalScreen(WifiUseCase* wifi, BuzzerUseCase* buzzer)
    : _wifi(wifi), _buzzer(buzzer) {}

void WifiPortalScreen::enter() {
    _state     = St::Serving;
    _done      = false;
    _attempted = false;
    _failed    = false;
    if (_buzzer) _buzzer->stopClick();      // kill the menu click before blocking
    _wifi->startAP();
    _portal.begin([w = _wifi]() { return w->scan(); });  // blocking scan first
    if (_buzzer) _buzzer->playWifiTune();   // then the chime, so tick() can advance it
}

void WifiPortalScreen::exit() {
    _portal.stop();
    _wifi->stopAP();
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
    // Perform the (blocking) connect here, after drawing a "please wait" frame.
    if (_state == St::Connecting && !_attempted) {
        d.clear();
        d.drawText(0, 0, "Menyimpan & hubungkan");
        d.drawLine(0, 11, d.width() - 1, 11);
        d.drawText(0, 18, "Ke:");
        d.drawText(0, 30, _targetSsid.substring(0, 21));
        d.drawText(0, 48, "Mohon tunggu...");
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

    // Serving — show how to join and configure
    d.clear();
    d.drawText(0, 0, "Setup WiFi via HP");
    d.drawLine(0, 11, d.width() - 1, 11);
    d.drawText(0, 14, "1.WiFi HP ke:");
    d.drawText(0, 24, String("  ") + Config::Ap::SSID);
    d.drawText(0, 34, "2.Buka browser:");
    d.drawText(0, 44, String("  ") + _wifi->apIp());
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
