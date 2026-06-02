#pragma once
#include "../IScreen.h"
#include "../../application/BuzzerUseCase.h"

class SettingsScreen : public IScreen {
public:
    explicit SettingsScreen(BuzzerUseCase* buzzer);
    void enter() override;
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    BuzzerUseCase*  _buzzer;
    ThresholdConfig _cfg;
    int             _field = 0;
    bool            _done  = false;

    struct Field {
        const char* label;
        float*      val;
        float       step;
        float       min;
        float       max;
    };
    // Alarm, 5 value fields, then the two action rows below.
    static constexpr int FIELD_COUNT  = 8;
    static constexpr int SAVE_FIELD   = 6;  // "Simpan"
    static constexpr int CANCEL_FIELD = 7;  // "Batal"
    Field _fields[FIELD_COUNT];
};
