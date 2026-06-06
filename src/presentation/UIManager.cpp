#include "UIManager.h"

UIManager::UIManager(IDisplay* display, SensorUseCase* sensor,
                     WifiUseCase* wifi, BuzzerUseCase* buzzer, StockUseCase* stock,
                     PowerUseCase* power, Gy271Compass* compass)
    : _display(display)
    , _sensor(sensor)
    , _wifi(wifi)
    , _buzzer(buzzer)
    , _stock(stock)
    , _power(power)
    , _mainScreen(sensor, wifi, buzzer, power)
    , _wifiScanScreen(wifi)
    , _wifiPortalScreen(wifi, buzzer)
    , _sensorScreen(sensor)
    , _settingsScreen(buzzer, power)
    , _stockScreen(stock, wifi, buzzer)
    , _compassScreen(compass)
{}

UIManager::~UIManager() {
    delete _wifiPassScreen;
}

void UIManager::begin() {
    _input.begin();
    _mainScreen.enter();
}

// Call every loop iteration. Button sampling happens in a background task; here
// we just drain every event it queued since the last frame (so no press is lost
// even when a single loop iteration was slow), dispatch each to the active
// screen, and act on any navigation it requested.
void UIManager::pollButtons() {
    if (_buzzer) _buzzer->tick();  // stop click sound when 20ms timer expires

    // Power-save: an active alarm keeps the screen awake; otherwise the eco
    // inactivity timers may dim/blank the panel.
    if (_power) _power->tick(_buzzer && _buzzer->isTriggered());

    // Per-loop background work for the active screen (e.g. the WiFi web portal).
    if (IScreen* scr = currentScreen()) scr->tick();

    bool acted = false;
    ButtonEvent evt;
    while ((evt = _input.poll()) != ButtonEvent::None) {
        // If the panel is asleep (eco), the first press only wakes it — swallow
        // the rest of this batch so a "wake" tap doesn't also trigger an action.
        if (_power && !_power->screenOn()) {
            _power->wake();
            _input.resetAll();
            acted = true;
            break;
        }
        if (_power) _power->wake();  // any press counts as activity

        // Non-blocking click feedback — starts tone, tick() stops it later
        if (_buzzer && !_buzzer->isTriggered())
            _buzzer->click();
        IScreen* scr = currentScreen();
        if (scr) scr->onButton(evt);
        // Resolve navigation per event so a transition's resetAll() can discard
        // any remaining queued presses that belonged to the previous screen.
        _handleNavigation();
        acted = true;
    }

    // Redraw the moment something changed instead of waiting for the 100ms
    // display timer — the screen now follows the button without lag.
    if (acted) render();
}

// Call on 100ms timer — does I2C display transfer only
void UIManager::render() {
    if (_power && !_power->screenOn()) return;  // panel off in eco — skip I2C flush
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
                case MainScreen::NavTo::WifiScan:   transitionTo(AppScreen::WifiScan);   break;
                case MainScreen::NavTo::WifiPortal: transitionTo(AppScreen::WifiPortal); break;
                case MainScreen::NavTo::Sensor:     transitionTo(AppScreen::Sensor);     break;
                case MainScreen::NavTo::Compass:    transitionTo(AppScreen::Compass);    break;
                case MainScreen::NavTo::Stock:      transitionTo(AppScreen::Stock);      break;
                case MainScreen::NavTo::Settings:   transitionTo(AppScreen::Settings);   break;
                default: break;
            }
        }
    } else {
        _handleDone();
    }
}

void UIManager::transitionTo(AppScreen next) {
    if (IScreen* prev = currentScreen()) prev->exit();  // let a screen release resources (AP/web)
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
            if (_buzzer) _buzzer->stopClick();  // avoid a stuck click during blocking connect
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
        case AppScreen::WifiPortal:   return &_wifiPortalScreen;
        case AppScreen::Sensor:       return &_sensorScreen;
        case AppScreen::Compass:      return &_compassScreen;
        case AppScreen::Stock:        return &_stockScreen;
        case AppScreen::Settings:     return &_settingsScreen;
    }
    return nullptr;
}
