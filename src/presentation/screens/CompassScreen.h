#pragma once
#include "../IScreen.h"
#include "../../infrastructure/sensors/Gy271Compass.h"

// Full-screen compass: a rose with N/E/S/W marks and a needle pointing to the
// current heading, plus a numeric degrees + cardinal readout. Reads the GY-271
// directly on tick() so the needle tracks smoothly (the shared sensor loop only
// samples every couple of seconds).
class CompassScreen : public IScreen {
public:
    explicit CompassScreen(Gy271Compass* compass);
    void enter() override;
    void tick()  override;
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    Gy271Compass* _compass;
    float    _heading    = -1.0f;  // <0 = no reading yet / sensor absent
    uint32_t _lastReadMs = 0;
    bool     _done       = false;
};
