#pragma once
#include "../IScreen.h"
#include "../../application/StockUseCase.h"
#include "../../application/WifiUseCase.h"
#include "../../application/BuzzerUseCase.h"

// Shows the IHSG index value, its up/down change, and an intraday line graph.
// Refreshes on a timer while open; Center forces a refresh, Left goes back.
class StockScreen : public IScreen {
public:
    StockScreen(StockUseCase* stock, WifiUseCase* wifi, BuzzerUseCase* buzzer);
    void enter() override;
    void tick()  override;
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    void _drawData(IDisplay& d, const StockData& s);
    void _drawGraph(IDisplay& d, const StockData& s, int top, int bottom);

    StockUseCase*  _stock;
    WifiUseCase*   _wifi;
    BuzzerUseCase* _buzzer;
    bool _done      = false;
    bool _needFetch = false;
    bool _announce  = false;  // play the load chime after the next fetch
};
