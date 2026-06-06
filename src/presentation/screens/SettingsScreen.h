#pragma once
#include "../IScreen.h"
#include "../../application/BuzzerUseCase.h"
#include "../../application/PowerUseCase.h"

class SettingsScreen : public IScreen {
public:
    SettingsScreen(BuzzerUseCase* buzzer, PowerUseCase* power);
    void enter() override;
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    BuzzerUseCase*  _buzzer;
    PowerUseCase*   _power;
    ThresholdConfig _cfg;
    bool            _eco   = false;  // local copy, applied on Save
    int             _field = 0;
    bool            _done  = false;

    struct Field {
        const char* label;
        float*      val;
        float       step;
        float       min;
        float       max;
    };
    // Alarm toggle, Eco toggle, 5 value fields, then the two action rows below.
    static constexpr int FIELD_COUNT  = 9;
    static constexpr int ALARM_FIELD  = 0;  // "Alarm"  (toggle)
    static constexpr int ECO_FIELD    = 1;  // "Hemat"  (toggle)
    static constexpr int SAVE_FIELD   = 7;  // "Simpan"
    static constexpr int CANCEL_FIELD = 8;  // "Batal"
    Field _fields[FIELD_COUNT];
};
