#pragma once
#include "../IScreen.h"
#include "../../application/SensorUseCase.h"
#include "../../application/WifiUseCase.h"
#include "../../application/BuzzerUseCase.h"
#include "../../application/PowerUseCase.h"

class MainScreen : public IScreen {
public:
    enum class NavTo { None, WifiPortal, Sensor, Compass, Stock, Forex, Settings };

    MainScreen(SensorUseCase* sensor, WifiUseCase* wifi, BuzzerUseCase* buzzer,
               PowerUseCase* power);
    void  enter() override;
    void  update(IDisplay& display) override;
    void  onButton(ButtonEvent evt) override;
    NavTo navTo() const   { return _navTo; }
    void  clearNav()      { _navTo = NavTo::None; }
    void  enterMenu()     { _menuMode = true; _menuIdx = 0; }

private:
    void drawHeader(IDisplay& d, const SensorData& data);
    void drawSummary(IDisplay& d, const SensorData& data);
    void drawMenu(IDisplay& d);
    // Footer bar with one label per physical button (left/center/right),
    // each aligned over its button so on-screen hints map to the hardware.
    void drawButtonBar(IDisplay& d, const char* left, const char* center,
                       const char* right);

    SensorUseCase* _sensor;
    WifiUseCase*   _wifi;
    BuzzerUseCase* _buzzer;
    PowerUseCase*  _power;
    NavTo          _navTo    = NavTo::None;
    bool           _menuMode = false;
    int            _menuIdx  = 0;
};
