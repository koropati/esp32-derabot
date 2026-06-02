#pragma once
#include "InputHandler.h"
#include "screens/MainScreen.h"
#include "screens/WifiScanScreen.h"
#include "screens/WifiPasswordScreen.h"
#include "screens/WifiPortalScreen.h"
#include "screens/SensorScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/StockScreen.h"
#include "../domain/interfaces/IDisplay.h"
#include "../application/SensorUseCase.h"
#include "../application/WifiUseCase.h"
#include "../application/BuzzerUseCase.h"
#include "../application/StockUseCase.h"

enum class AppScreen { Main, WifiScan, WifiPassword, WifiPortal, Sensor, Stock, Settings };

class UIManager {
public:
    UIManager(IDisplay*      display,
              SensorUseCase* sensor,
              WifiUseCase*   wifi,
              BuzzerUseCase* buzzer,
              StockUseCase*  stock);
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

    InputHandler        _input;
    AppScreen           _current = AppScreen::Main;

    MainScreen          _mainScreen;
    WifiScanScreen      _wifiScanScreen;
    WifiPasswordScreen* _wifiPassScreen = nullptr;
    WifiPortalScreen    _wifiPortalScreen;
    SensorScreen        _sensorScreen;
    SettingsScreen      _settingsScreen;
    StockScreen         _stockScreen;
};
