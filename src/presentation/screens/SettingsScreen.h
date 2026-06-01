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
    static constexpr int FIELD_COUNT = 7;
    Field _fields[FIELD_COUNT];
};
