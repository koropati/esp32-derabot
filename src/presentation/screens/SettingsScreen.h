#pragma once
#include "../IScreen.h"
#include "../../application/BuzzerUseCase.h"
#include "../../application/PowerUseCase.h"
#include "../../application/StockUseCase.h"

class SettingsScreen : public IScreen {
public:
    SettingsScreen(BuzzerUseCase* buzzer, PowerUseCase* power, StockUseCase* stock);
    void enter() override;
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    BuzzerUseCase*  _buzzer;
    PowerUseCase*   _power;
    StockUseCase*   _stock;
    ThresholdConfig _cfg;
    bool            _eco        = false;  // local copy, applied on Save
    bool            _motionWake = true;   // local copy, applied on Save
    int             _stockIdx   = 0;      // local copy of the market code selection
    int             _field      = 0;
    bool            _done       = false;

    struct Field {
        const char* label;
        float*      val;
        float       step;
        float       min;
        float       max;
    };
    // Alarm/Eco/WakeGerak toggles, Saham selector, 7 value fields, then the two
    // action rows below.
    static constexpr int FIELD_COUNT  = 13;
    static constexpr int ALARM_FIELD  = 0;   // "Alarm"     (toggle)
    static constexpr int ECO_FIELD    = 1;   // "Hemat"     (toggle)
    static constexpr int WAKE_FIELD   = 2;   // "WakeGerak" (toggle)
    static constexpr int SAHAM_FIELD  = 3;   // "Saham"     (selector)
    static constexpr int SAVE_FIELD   = 11;  // "Simpan"
    static constexpr int CANCEL_FIELD = 12;  // "Batal"
    Field _fields[FIELD_COUNT];
};
