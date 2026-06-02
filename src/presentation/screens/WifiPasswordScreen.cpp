#include "WifiPasswordScreen.h"

constexpr const char WifiPasswordScreen::CHARSET[];

WifiPasswordScreen::WifiPasswordScreen(const String& ssid) : _ssid(ssid) {}

void WifiPasswordScreen::enter() {
    _password  = "";
    _pickIdx   = 0;
    _done      = false;
    _confirmed = false;
}

// Short label shown for a picker position: a real character, or an action token.
String WifiPasswordScreen::_pickLabel(int idx) const {
    idx = (idx % PICK_LEN + PICK_LEN) % PICK_LEN;
    if (idx == IDX_DEL) return "DEL";
    if (idx == IDX_OK)  return "OK";
    return String(CHARSET[idx]);
}

void WifiPasswordScreen::update(IDisplay& d) {
    d.clear();

    // Title
    d.drawText(0, 0, "WiFi Password");
    String ssidLine = _ssid;
    if (ssidLine.length() > 21) ssidLine = ssidLine.substring(0, 20) + "~";
    d.drawText(0, 9, ssidLine);
    d.drawLine(0, 18, d.width() - 1, 18);

    // Password display (last 19 chars + cursor)
    String pw = _password;
    if (pw.length() > 19) pw = pw.substring(pw.length() - 19);
    d.drawText(0, 21, pw + "_");

    d.drawLine(0, 32, d.width() - 1, 32);

    // Character / action picker: show prev / current / next
    String prev = _pickLabel(_pickIdx - 1);
    String curr = _pickLabel(_pickIdx);
    String next = _pickLabel(_pickIdx + 1);
    d.drawText(8, 37, prev + "  [" + curr + "]  " + next);

    d.drawLine(0, 47, d.width() - 1, 47);

    // Footer hints
    d.drawText(0, 50, "< >:Pilih  Ctr:Tekan");
    d.drawText(0, 57, "DEL:Hapus  OK:Simpan");

    d.flush();
}

void WifiPasswordScreen::onButton(ButtonEvent evt) {
    switch (evt) {
        case ButtonEvent::Left:
            _pickIdx = (_pickIdx - 1 + PICK_LEN) % PICK_LEN;
            break;
        case ButtonEvent::Right:
            _pickIdx = (_pickIdx + 1) % PICK_LEN;
            break;
        case ButtonEvent::Center:
            if (_pickIdx == IDX_DEL) {
                if (!_password.isEmpty())
                    _password.remove(_password.length() - 1);
            } else if (_pickIdx == IDX_OK) {
                // Submit when something was typed; empty OK = cancel / go back
                _confirmed = !_password.isEmpty();
                _done      = true;
            } else {
                _password += CHARSET[_pickIdx];
            }
            break;
        default:
            break;
    }
}
