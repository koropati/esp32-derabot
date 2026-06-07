#pragma once
#include "../IScreen.h"
#include "../../application/ForexUseCase.h"
#include "../../application/WifiUseCase.h"
#include "../../application/BuzzerUseCase.h"

// Shows the current rupiah (IDR) exchange rate for a list of currencies.
// Refreshes on a timer while open; Center forces a refresh, Left goes back.
class ForexScreen : public IScreen {
public:
    ForexScreen(ForexUseCase* forex, WifiUseCase* wifi, BuzzerUseCase* buzzer);
    void enter() override;
    void tick()  override;
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    void _drawData(IDisplay& d, const ExchangeData& s);

    static constexpr int PER_PAGE = 3;   // rows per screen (kept roomy/legible)

    ForexUseCase*  _forex;
    WifiUseCase*   _wifi;
    BuzzerUseCase* _buzzer;
    bool _done      = false;
    bool _needFetch = false;
    bool _announce  = false;  // play the load chime after the next fetch
    int  _page      = 0;      // current page when the list spans multiple screens
};
