#pragma once
#include "IScreen.h"
#include <cstdint>

class InputHandler {
public:
    void        begin();
    ButtonEvent poll();
    void        resetAll();  // call after screen transition — waits for release

private:
    enum class Phase : uint8_t {
        Idle,         // waiting for press
        Debounce,     // press detected, confirming it's real (not bounce)
        Pressed,      // confirmed press, waiting for release or long-press
        LongHeld,     // long press fired, waiting for physical release
        WaitRelease,  // post-transition guard: ignore input until button released
    };

    struct BtnState {
        Phase    phase  = Phase::Idle;
        uint32_t downAt = 0;
    };

    BtnState    _left, _center, _right;
    ButtonEvent _processPin(int pin, BtnState& s);
};
