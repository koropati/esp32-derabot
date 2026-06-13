#pragma once
#include "InputHandler.h"
#include "screens/MainScreen.h"
#include "screens/WifiPortalScreen.h"
#include "screens/SensorScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/StockScreen.h"
#include "screens/ForexScreen.h"
#include "screens/CompassScreen.h"
#include "screens/GameScreen.h"
#include "../domain/interfaces/IDisplay.h"
#include "../application/SensorUseCase.h"
#include "../application/WifiUseCase.h"
#include "../application/BuzzerUseCase.h"
#include "../application/StockUseCase.h"
#include "../application/ForexUseCase.h"
#include "../application/PowerUseCase.h"

enum class AppScreen { Main, WifiPortal, Sensor, Compass, Stock, Forex, Game, Settings };

class UIManager {
public:
    UIManager(IDisplay*      display,
              SensorUseCase* sensor,
              WifiUseCase*   wifi,
              BuzzerUseCase* buzzer,
              StockUseCase*  stock,
              ForexUseCase*  forex,
              PowerUseCase*  power,
              Gy271Compass*  compass,
              IStorage*      storage);
    ~UIManager();
    void begin();
    void pollButtons();  // call every loop iteration — fast, no I2C
    void render();       // call on timer (100ms) — does I2C display transfer
    void update();       // convenience: pollButtons + render (legacy)

private:
    void      _handleNavigation();
    void      _handleDone();
    void      transitionTo(AppScreen next);
    IScreen*  currentScreen();

    IDisplay*       _display;
    SensorUseCase*  _sensor;
    WifiUseCase*    _wifi;
    BuzzerUseCase*  _buzzer;
    StockUseCase*   _stock;
    ForexUseCase*   _forex;
    PowerUseCase*   _power;

    InputHandler        _input;
    AppScreen           _current = AppScreen::Main;

    MainScreen          _mainScreen;
    WifiPortalScreen    _wifiPortalScreen;
    SensorScreen        _sensorScreen;
    SettingsScreen      _settingsScreen;
    StockScreen         _stockScreen;
    ForexScreen         _forexScreen;
    CompassScreen       _compassScreen;
    GameScreen          _gameScreen;
};
