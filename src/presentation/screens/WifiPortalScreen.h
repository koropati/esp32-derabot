#pragma once
#include "../IScreen.h"
#include "../../application/WifiUseCase.h"
#include "../../application/BuzzerUseCase.h"
#include "../../infrastructure/wifi/WifiPortal.h"

// Brings up the soft-AP config portal and shows join instructions on the OLED.
// When the user submits credentials from their phone, it saves + connects and
// reports the result. Back (Left) tears the portal down and returns to the menu.
class WifiPortalScreen : public IScreen {
public:
    WifiPortalScreen(WifiUseCase* wifi, BuzzerUseCase* buzzer);
    void enter() override;
    void exit()  override;
    void tick()  override;                 // service the web server every loop
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    enum class St { Serving, Connecting, Success };

    WifiUseCase*   _wifi;
    BuzzerUseCase* _buzzer;
    WifiPortal     _portal;
    St     _state     = St::Serving;
    bool   _done      = false;
    bool   _attempted = false;   // connect already tried for current submission
    bool   _failed    = false;   // last connect attempt failed
    String _targetSsid;
    String _targetPass;
};
