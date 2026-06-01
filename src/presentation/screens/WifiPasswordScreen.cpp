#include "WifiPasswordScreen.h"

constexpr const char WifiPasswordScreen::CHARSET[];

WifiPasswordScreen::WifiPasswordScreen(const String& ssid) : _ssid(ssid) {}

void WifiPasswordScreen::enter() {
    _password  = "";
    _charIdx   = 0;
    _done      = false;
    _confirmed = false;
}

void WifiPasswordScreen::update(IDisplay& d) {
    d.clear();

    // Title
    d.drawText(0, 0, "WiFi Password");
    String ssidLine = _ssid;
    if (ssidLine.length() > 21) ssidLine = ssidLine.substring(0, 20) + "~";
    d.drawText(0, 9, ssidLine);
    d.drawLine(0, 18, d.width() - 1, 18);

    // Password display (last 20 chars + cursor)
    String pw = _password;
    if (pw.length() > 19) pw = pw.substring(pw.length() - 19);
    d.drawText(0, 21, pw + "_");

    d.drawLine(0, 32, d.width() - 1, 32);

    // Character picker: show prev / current / next
    char prev = CHARSET[(_charIdx - 1 + CHARSET_LEN) % CHARSET_LEN];
    char curr = CHARSET[_charIdx % CHARSET_LEN];
    char next = CHARSET[(_charIdx + 1) % CHARSET_LEN];
    char buf[22];
    snprintf(buf, sizeof(buf), " %c  [%c]  %c", prev, curr, next);
    d.drawText(20, 37, buf);

    d.drawLine(0, 47, d.width() - 1, 47);

    // Footer hints
    d.drawText(0, 50, "< >:Scroll  Ctr:Add");
    d.drawText(0, 57, "LongCtr:Submit  Del?");

    d.flush();
}

void WifiPasswordScreen::onButton(ButtonEvent evt) {
    switch (evt) {
        case ButtonEvent::Left:
            _charIdx = (_charIdx - 1 + CHARSET_LEN) % CHARSET_LEN;
            break;
        case ButtonEvent::Right:
            _charIdx = (_charIdx + 1) % CHARSET_LEN;
            break;
        case ButtonEvent::Center:
            _password += CHARSET[_charIdx % CHARSET_LEN];
            break;
        case ButtonEvent::LongCenter:
            if (!_password.isEmpty()) {
                _confirmed = true;
                _done      = true;
            } else {
                // Empty password = cancel / go back
                _confirmed = false;
                _done      = true;
            }
            break;
        default:
            break;
    }
}
