#pragma once
#include "../IScreen.h"
#include "../../application/SensorUseCase.h"
#include "../../application/WifiUseCase.h"
#include "../../application/BuzzerUseCase.h"

class MainScreen : public IScreen {
public:
    enum class NavTo { None, WifiScan, WifiPortal, Sensor, Stock, Settings };

    MainScreen(SensorUseCase* sensor, WifiUseCase* wifi, BuzzerUseCase* buzzer);
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

    SensorUseCase* _sensor;
    WifiUseCase*   _wifi;
    BuzzerUseCase* _buzzer;
    NavTo          _navTo    = NavTo::None;
    bool           _menuMode = false;
    int            _menuIdx  = 0;
};
