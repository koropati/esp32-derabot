#include "MainScreen.h"
#include <Arduino.h>

static const char* MENU_ITEMS[] = {
    "WiFi via HP", "Sensor Detail", "Compass", "Bursa Saham", "Kurs Rupiah", "Main Game", "Settings"
};
static constexpr int MENU_COUNT = 7;

MainScreen::MainScreen(SensorUseCase* sensor, WifiUseCase* wifi, BuzzerUseCase* buzzer,
                       PowerUseCase* power, Gy271Compass* compass)
    : _sensor(sensor)
    , _wifi(wifi)
    , _buzzer(buzzer)
    , _power(power)
    , _compass(compass)
    , _mochi(buzzer, power, compass)
{}

void MainScreen::enter() {
    _menuMode = false;
    _navTo    = NavTo::None;
    _mochi.begin();
    _buttonPressedThisFrame = false;
}

void MainScreen::tick() {
    bool pressed = _buttonPressedThisFrame;
    _buttonPressedThisFrame = false;
    
    // Update Mochi Face logic (shake, sensors, and wifi status)
    // Only enable shake detection if NOT in menu mode
    _mochi.tick(_sensor->lastData(), pressed, _wifi->isConnected(), !_menuMode);
}

void MainScreen::update(IDisplay& d) {
    d.clear();
    
    if (_menuMode) {
        const SensorData& data = _sensor->lastData();
        drawHeader(d, data);
        d.drawLine(0, 12, d.width() - 1, 12);
        drawMenu(d);
    } else {
        // Draw full-screen Mochi Face animation
        _mochi.draw(d);
    }
    
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

void MainScreen::drawMenu(IDisplay& d) {
    // Roomy rows for legibility: only 4 fit between the header divider (y12) and
    // the hint bar (y51), so scroll a window to keep the cursor visible.
    constexpr int VISIBLE = 4;
    constexpr int START_Y = 15;
    constexpr int STEP    = 9;
    int start = (_menuIdx >= VISIBLE) ? _menuIdx - VISIBLE + 1 : 0;
    for (int row = 0; row < VISIBLE; row++) {
        int idx = start + row;
        if (idx >= MENU_COUNT) break;
        int y = START_Y + row * STEP;
        if (idx == _menuIdx)
            d.drawRect(0, y - 1, d.width() - 1, 9, false);  // selection box
        d.drawText(4, y, MENU_ITEMS[idx]);
    }
    // Scroll arrows on the right edge when the list extends beyond this page.
    if (start > 0)                 d.drawText(d.width() - 6, START_Y, "^");
    if (start + VISIBLE < MENU_COUNT)
        d.drawText(d.width() - 6, START_Y + (VISIBLE - 1) * STEP, "v");
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
    _buttonPressedThisFrame = true;
    
    // If Mochi is sleeping, any button wakes him up
    if (_mochi.getState() == MochiState::Sleeping) {
        _mochi.wakeUp();
        if (evt == ButtonEvent::Center) {
            _menuMode = true;
            _menuIdx  = 0;
        }
        return;
    }
    
    if (!_menuMode) {
        if (evt == ButtonEvent::Center) {
            _menuMode = true;     // Center always opens menu
            _menuIdx  = 0;
        } else if (evt == ButtonEvent::Left || evt == ButtonEvent::Right) {
            // Left/Right button click in home page: Mochi gets happy and smiles!
            _mochi.setState(MochiState::Happy);
        }
        return;
    }
    
    switch (evt) {
        case ButtonEvent::Left:
            if (_menuIdx == 0)
                _menuMode = false;  // Left at top item = exit menu (back to Mochi face)
            else
                _menuIdx--;
            break;
        case ButtonEvent::Right:
            _menuIdx = (_menuIdx + 1) % MENU_COUNT;
            break;
        case ButtonEvent::Center:
            Serial.printf("[MainScreen] Center clicked, _menuIdx=%d, MENU_COUNT=%d\n", _menuIdx, MENU_COUNT);
            if (_menuIdx == 0) _navTo = NavTo::WifiPortal;
            if (_menuIdx == 1) _navTo = NavTo::Sensor;
            if (_menuIdx == 2) _navTo = NavTo::Compass;
            if (_menuIdx == 3) _navTo = NavTo::Stock;
            if (_menuIdx == 4) _navTo = NavTo::Forex;
            if (_menuIdx == 5) _navTo = NavTo::Game;
            if (_menuIdx == 6) _navTo = NavTo::Settings;
            break;
        default:
            break;
    }
}
