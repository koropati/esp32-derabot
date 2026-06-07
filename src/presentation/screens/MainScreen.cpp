#include "MainScreen.h"
#include <Arduino.h>

static const char* MENU_ITEMS[] = {
    "WiFi via HP", "Sensor Detail", "Compass", "Bursa IHSG", "Settings"
};
static constexpr int MENU_COUNT = 5;

MainScreen::MainScreen(SensorUseCase* sensor, WifiUseCase* wifi, BuzzerUseCase* buzzer,
                       PowerUseCase* power)
    : _sensor(sensor), _wifi(wifi), _buzzer(buzzer), _power(power) {}

void MainScreen::enter() {
    _menuMode = false;
    _navTo    = NavTo::None;
}

void MainScreen::update(IDisplay& d) {
    const SensorData& data = _sensor->lastData();
    d.clear();
    drawHeader(d, data);
    d.drawLine(0, 12, d.width() - 1, 12);

    if (_menuMode)
        drawMenu(d);
    else
        drawSummary(d, data);
    d.flush();
}

void MainScreen::drawHeader(IDisplay& d, const SensorData& data) {
    // Signal strength bars (4 bars, leftmost)
    for (int i = 0; i < 4; i++) {
        int barH = 3 + i * 2;
        int barX = 2 + i * 4;
        int barY = 10 - barH;
        bool lit = _wifi->isConnected() && data.signalPct >= (i + 1) * 25;
        d.drawRect(barX, barY, 3, barH, lit);
    }

    // Centre: ECO badge while power-save is active, else the brand name.
    if (_power && _power->active()) {
        d.drawRect(40, 1, 23, 10, false);
        d.drawText(43, 2, "ECO");
    } else {
        d.drawText(30, 2, "DeraBot");
    }

    // Battery: percentage text then an icon with a proportional fill.
    char batStr[6];
    snprintf(batStr, sizeof(batStr), "%d%%", data.batteryPct);
    d.drawText(101 - 6 * (int)strlen(batStr), 2, batStr);

    int batW = map(data.batteryPct, 0, 100, 0, 12);
    d.drawRect(103, 2, 14, 8, false);   // battery outline
    d.drawRect(117, 4, 2, 4, true);     // battery terminal
    if (batW > 0) d.drawRect(104, 3, batW, 6, true);  // battery fill
}

void MainScreen::drawSummary(IDisplay& d, const SensorData& data) {
    char buf[24];

    // Hero: temperature in large type with a small degree mark + unit.
    snprintf(buf, sizeof(buf), "%.1f", data.temperature);
    d.drawText(2, 15, buf, 2);
    int tx = 2 + 12 * (int)strlen(buf);     // pixel just past the big number
    d.drawRect(tx + 1, 15, 3, 3, false);    // degree ring
    d.drawText(tx + 6, 16, "C");

    // Right column: humidity + pressure, stacked.
    snprintf(buf, sizeof(buf), "RH:%.0f%%", data.humidity);
    d.drawText(80, 15, buf);
    snprintf(buf, sizeof(buf), "%.0fhPa", data.pressure);
    d.drawText(80, 25, buf);

    // Secondary row: sound + voltage, or an alarm banner.
    if (_buzzer->isTriggered()) {
        d.drawRect(0, 33, d.width() - 1, 11, false);
        d.drawText(6, 35, "!! ALARM AKTIF !!");
    } else {
        snprintf(buf, sizeof(buf), "Suara:%.1fdB", data.soundDb);
        d.drawText(2, 34, buf);
        snprintf(buf, sizeof(buf), "%.2fV", data.voltage);
        d.drawText(92, 34, buf);
    }

    // WiFi status line — IP when online, plain hint otherwise.
    if (_wifi->isConnected()) {
        snprintf(buf, sizeof(buf), "WiFi %s", _wifi->getIp().c_str());
        d.drawText(0, 43, String(buf).substring(0, 21).c_str());
    } else {
        d.drawText(0, 43, "WiFi: belum konek");
    }

    // Footer: button hints aligned to the three physical buttons. Only the
    // Center button opens the menu, so the "Menu" label sits in the centre
    // (boxed like a key) directly above it — Left mutes an active alarm.
    drawButtonBar(d, _buzzer->isTriggered() ? "Mute" : "", "Menu", "");
}

void MainScreen::drawMenu(IDisplay& d) {
    // 5 items fit between the header divider (y12) and the hint bar (y51);
    // 7px rows keep the last selection box clear of the bar's divider.
    for (int i = 0; i < MENU_COUNT; i++) {
        int y = 14 + i * 7;
        if (i == _menuIdx)
            d.drawRect(0, y - 1, d.width() - 1, 9, false);  // selection box
        d.drawText(3, y, MENU_ITEMS[i]);
    }
    // Button hints mapped to the three physical buttons below the screen.
    drawButtonBar(d, "Atas", "Pilih", "Bawah");
}

// One label per physical button, each aligned over its button: left flush
// left, right flush right, center centred and boxed so it reads as a key.
void MainScreen::drawButtonBar(IDisplay& d, const char* left, const char* center,
                               const char* right) {
    const int y = 55;
    d.drawLine(0, 51, d.width() - 1, 51);
    if (left && *left)
        d.drawText(0, y, left);
    if (right && *right)
        d.drawText(d.width() - 6 * (int)strlen(right), y, right);
    if (center && *center) {
        int w = 6 * (int)strlen(center);
        int x = (d.width() - w) / 2;
        d.drawRect(x - 3, y - 2, w + 5, 11, false);
        d.drawText(x, y, center);
    }
}

void MainScreen::onButton(ButtonEvent evt) {
    if (!_menuMode) {
        if (evt == ButtonEvent::Left && _buzzer->isTriggered()) {
            _buzzer->silence();   // Left silences alarm from main view
            return;
        }
        if (evt == ButtonEvent::Center) {
            _menuMode = true;     // Center always opens menu
            _menuIdx  = 0;
        }
        return;
    }
    switch (evt) {
        case ButtonEvent::Left:
            if (_menuIdx == 0)
                _menuMode = false;  // Left at top item = exit menu
            else
                _menuIdx--;
            break;
        case ButtonEvent::Right:
            _menuIdx = (_menuIdx + 1) % MENU_COUNT;
            break;
        case ButtonEvent::Center:
            if (_menuIdx == 0) _navTo = NavTo::WifiPortal;
            if (_menuIdx == 1) _navTo = NavTo::Sensor;
            if (_menuIdx == 2) _navTo = NavTo::Compass;
            if (_menuIdx == 3) _navTo = NavTo::Stock;
            if (_menuIdx == 4) _navTo = NavTo::Settings;
            break;
        default:
            break;
    }
}
