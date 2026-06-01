#include "UIManager.h"

UIManager::UIManager(IDisplay* display, SensorUseCase* sensor,
                     WifiUseCase* wifi, BuzzerUseCase* buzzer)
    : _display(display)
    , _sensor(sensor)
    , _wifi(wifi)
    , _buzzer(buzzer)
    , _mainScreen(sensor, wifi, buzzer)
    , _wifiScanScreen(wifi)
    , _sensorScreen(sensor)
    , _settingsScreen(buzzer)
{}

UIManager::~UIManager() {
    delete _wifiPassScreen;
}

void UIManager::begin() {
    _input.begin();
    _mainScreen.enter();
}

// Call every loop iteration — reads GPIO (fast), then checks navigation
void UIManager::pollButtons() {
    if (_buzzer) _buzzer->tick();  // stop click sound when 20ms timer expires
    ButtonEvent evt = _input.poll();
    if (evt != ButtonEvent::None) {
        // Non-blocking click feedback — starts tone, tick() stops it later
        if (_buzzer && !_buzzer->isTriggered())
            _buzzer->click();
        IScreen* scr = currentScreen();
        if (scr) scr->onButton(evt);
    }
    // Always check navigation — not only on button events
    _handleNavigation();
}

// Call on 100ms timer — does I2C display transfer only
void UIManager::render() {
    IScreen* scr = currentScreen();
    if (scr) scr->update(*_display);
}

// Legacy wrapper
void UIManager::update() {
    pollButtons();
    render();
}

void UIManager::_handleNavigation() {
    if (_current == AppScreen::Main) {
        auto nav = _mainScreen.navTo();
        if (nav != MainScreen::NavTo::None) {
            _mainScreen.clearNav();
            switch (nav) {
                case MainScreen::NavTo::WifiScan: transitionTo(AppScreen::WifiScan);  break;
                case MainScreen::NavTo::Sensor:   transitionTo(AppScreen::Sensor);    break;
                case MainScreen::NavTo::Settings: transitionTo(AppScreen::Settings);  break;
                default: break;
            }
        }
    } else {
        _handleDone();
    }
}

void UIManager::transitionTo(AppScreen next) {
    _current = next;
    IScreen* s = currentScreen();
    if (s) s->enter();
    _input.resetAll();  // clear accumulated state from any blocking in enter()
}

void UIManager::_handleDone() {
    IScreen* scr = currentScreen();
    if (!scr || !scr->isDone()) return;

    if (_current == AppScreen::WifiScan) {
        if (_wifiScanScreen.wantConnect()) {
            // User selected a network — go to password entry
            delete _wifiPassScreen;
            _wifiPassScreen = new WifiPasswordScreen(_wifiScanScreen.selectedSsid());
            transitionTo(AppScreen::WifiPassword);
            return;
        }
        // User pressed back — return to menu
        transitionTo(AppScreen::Main);
        _mainScreen.enterMenu();

    } else if (_current == AppScreen::WifiPassword) {
        if (_wifiPassScreen && _wifiPassScreen->confirmed()) {
            // Attempt connection — return to main (not menu) after action
            _display->clear();
            _display->drawText(15, 20, "Menghubungkan...");
            _display->flush();
            _wifi->connect(_wifiScanScreen.selectedSsid(),
                           _wifiPassScreen->password());
            transitionTo(AppScreen::Main);
        } else {
            // Cancelled — return to menu
            transitionTo(AppScreen::Main);
            _mainScreen.enterMenu();
        }

    } else {
        // SensorScreen, SettingsScreen: back always returns to menu
        transitionTo(AppScreen::Main);
        _mainScreen.enterMenu();
    }
}

IScreen* UIManager::currentScreen() {
    switch (_current) {
        case AppScreen::Main:         return &_mainScreen;
        case AppScreen::WifiScan:     return &_wifiScanScreen;
        case AppScreen::WifiPassword: return _wifiPassScreen;
        case AppScreen::Sensor:       return &_sensorScreen;
        case AppScreen::Settings:     return &_settingsScreen;
    }
    return nullptr;
}
