#include "MainScreen.h"
#include <Arduino.h>

static const char* MENU_ITEMS[] = {
    "WiFi via HP", "WiFi (Tombol)", "Sensor Detail", "Compass", "Bursa IHSG", "Settings"
};
static constexpr int MENU_COUNT = 6;

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
    char buf[22];

    // Hero: temperature in large type with a small degree mark + unit.
    snprintf(buf, sizeof(buf), "%.1f", data.temperature);
    d.drawText(2, 16, buf, 2);
    int tx = 2 + 12 * (int)strlen(buf);     // pixel just past the big number
    d.drawRect(tx + 1, 16, 3, 3, false);    // degree ring
    d.drawText(tx + 6, 17, "C");

    // Right column: humidity + pressure, stacked.
    snprintf(buf, sizeof(buf), "RH:%.0f%%", data.humidity);
    d.drawText(80, 15, buf);
    snprintf(buf, sizeof(buf), "%.0fhPa", data.pressure);
    d.drawText(80, 27, buf);

    if (_buzzer->isTriggered()) {
        // Alarm takes over the secondary row for visibility.
        d.drawRect(0, 36, d.width() - 1, 11, false);
        d.drawText(6, 38, "!! ALARM AKTIF !!");
    } else {
        snprintf(buf, sizeof(buf), "Suara:%.1fdB", data.soundDb);
        d.drawText(2, 38, buf);
        snprintf(buf, sizeof(buf), "%.2fV", data.voltage);
        d.drawText(92, 38, buf);
    }

    // Footer: status on the left, navigation hint on the right.
    d.drawLine(0, 50, d.width() - 1, 50);
    if (_buzzer->isTriggered()) {
        d.drawText(0, 54, "<Mute");
    } else if (_wifi->isConnected()) {
        d.drawText(0, 54, _wifi->getIp().substring(0, 15).c_str());
    } else {
        d.drawText(0, 54, "WiFi: -");
    }
    d.drawText(92, 54, "Menu >");
}

void MainScreen::drawMenu(IDisplay& d) {
    // 6 items fit between the header divider (y12) and the bottom edge.
    for (int i = 0; i < MENU_COUNT; i++) {
        int y = 14 + i * 8;
        if (i == _menuIdx)
            d.drawRect(0, y - 1, d.width() - 1, 9, false);  // selection box
        d.drawText(3, y, MENU_ITEMS[i]);
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
            if (_menuIdx == 1) _navTo = NavTo::WifiScan;
            if (_menuIdx == 2) _navTo = NavTo::Sensor;
            if (_menuIdx == 3) _navTo = NavTo::Compass;
            if (_menuIdx == 4) _navTo = NavTo::Stock;
            if (_menuIdx == 5) _navTo = NavTo::Settings;
            break;
        default:
            break;
    }
}
