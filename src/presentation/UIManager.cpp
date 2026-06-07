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
    , _wifiPortalScreen(wifi, buzzer, power)
    , _sensorScreen(sensor)
    , _settingsScreen(buzzer, power, stock)
    , _stockScreen(stock, wifi, buzzer)
    , _compassScreen(compass)
{}

UIManager::~UIManager() {}

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

    // Every sub-screen (WiFi portal, Sensor, Compass, Stock, Settings) returns
    // to the menu when it signals done (Back).
    transitionTo(AppScreen::Main);
    _mainScreen.enterMenu();
}

IScreen* UIManager::currentScreen() {
    switch (_current) {
        case AppScreen::Main:         return &_mainScreen;
        case AppScreen::WifiPortal:   return &_wifiPortalScreen;
        case AppScreen::Sensor:       return &_sensorScreen;
        case AppScreen::Compass:      return &_compassScreen;
        case AppScreen::Stock:        return &_stockScreen;
        case AppScreen::Settings:     return &_settingsScreen;
    }
    return nullptr;
}
