#include "MainScreen.h"
#include <Arduino.h>

static const char* MENU_ITEMS[] = { "WiFi Setup", "Sensor Detail", "Settings" };
static constexpr int MENU_COUNT = 3;

MainScreen::MainScreen(SensorUseCase* sensor, WifiUseCase* wifi, BuzzerUseCase* buzzer)
    : _sensor(sensor), _wifi(wifi), _buzzer(buzzer) {}

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
        int barY = 11 - barH;
        if (data.signalPct >= (i + 1) * 25)
            d.drawRect(barX, barY, 3, barH, true);
        else
            d.drawRect(barX, barY, 3, barH, false);
    }

    // WiFi connection dot
    if (_wifi->isConnected())
        d.drawRect(20, 4, 3, 3, true);

    // MQTT indicator
    char status[16];
    snprintf(status, sizeof(status), "DeraBot");
    d.drawText(26, 2, status);

    // Battery icon (right side)
    int batW = map(data.batteryPct, 0, 100, 0, 12);
    d.drawRect(105, 2, 14, 8, false);  // battery outline
    d.drawRect(119, 4, 2, 4, true);    // battery terminal
    d.drawRect(106, 3, batW, 6, true); // battery fill

    char batStr[5];
    snprintf(batStr, sizeof(batStr), "%d%%", data.batteryPct);
    d.drawText(d.width() - 6 * strlen(batStr) - 22, 2, batStr);
}

void MainScreen::drawSummary(IDisplay& d, const SensorData& data) {
    char buf[22];

    snprintf(buf, sizeof(buf), "T:%.1fC H:%.0f%%", data.temperature, data.humidity);
    d.drawText(0, 15, buf);

    snprintf(buf, sizeof(buf), "P:%.0fhPa", data.pressure);
    d.drawText(0, 25, buf);

    snprintf(buf, sizeof(buf), "dB:%.1f  V:%.2fV", data.soundDb, data.voltage);
    d.drawText(0, 35, buf);

    if (_buzzer->isTriggered())
        d.drawText(0, 46, "! ALARM aktif");
    else if (_wifi->isConnected())
        d.drawText(0, 46, _wifi->getIp().substring(0, 21).c_str());
    else
        d.drawText(0, 46, "WiFi: not connected");

    d.drawLine(0, 55, d.width() - 1, 55);
    if (_buzzer->isTriggered())
        d.drawText(0, 57, "<Mute  Menu  >");
    else
        d.drawText(0, 57, "      Menu >");
}

void MainScreen::drawMenu(IDisplay& d) {
    d.drawText(0, 14, "== MENU ==");
    for (int i = 0; i < MENU_COUNT; i++) {
        String line = (i == _menuIdx) ? "> " : "  ";
        line += MENU_ITEMS[i];
        d.drawText(0, 24 + i * 11, line);
    }
    d.drawLine(0, 55, d.width() - 1, 55);
    d.drawText(0, 57, "<Atas >Bwh  Ctr:Pilih");
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
            if (_menuIdx == 0) _navTo = NavTo::WifiScan;
            if (_menuIdx == 1) _navTo = NavTo::Sensor;
            if (_menuIdx == 2) _navTo = NavTo::Settings;
            break;
        case ButtonEvent::LongCenter:
            // intentionally ignored — menu stays open until explicit nav
            break;
        default:
            break;
    }
}
