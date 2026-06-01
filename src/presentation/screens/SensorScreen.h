#pragma once
#include "../IScreen.h"
#include "../../application/SensorUseCase.h"

class SensorScreen : public IScreen {
public:
    explicit SensorScreen(SensorUseCase* sensor);
    void enter() override;
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    SensorUseCase* _sensor;
    int            _page = 0;
    bool           _done = false;
};
