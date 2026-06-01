#include "InputHandler.h"
#include "../config/config.h"
#include <Arduino.h>

void InputHandler::begin() {
    pinMode(Config::Pins::BTN_LEFT,   INPUT_PULLUP);
    pinMode(Config::Pins::BTN_CENTER, INPUT_PULLUP);
    pinMode(Config::Pins::BTN_RIGHT,  INPUT_PULLUP);
}

ButtonEvent InputHandler::poll() {
    ButtonEvent e;
    e = _processPin(Config::Pins::BTN_LEFT,   _left);   if (e != ButtonEvent::None) return e;
    e = _processPin(Config::Pins::BTN_CENTER, _center); if (e != ButtonEvent::None) return e;
    e = _processPin(Config::Pins::BTN_RIGHT,  _right);  if (e != ButtonEvent::None) return e;
    return ButtonEvent::None;
}

// Called after every screen transition — prevents physical carry-over.
// Only puts a button into WaitRelease if it is currently being pressed;
// buttons that are already released go straight to Idle so the next
// press works without requiring an extra release cycle.
void InputHandler::resetAll() {
    auto init = [](int pin, BtnState& s) {
        s.phase = (digitalRead(pin) == LOW) ? Phase::WaitRelease : Phase::Idle;
    };
    init(Config::Pins::BTN_LEFT,   _left);
    init(Config::Pins::BTN_CENTER, _center);
    init(Config::Pins::BTN_RIGHT,  _right);
}

ButtonEvent InputHandler::_processPin(int pin, BtnState& s) {
    bool     raw = (digitalRead(pin) == LOW);
    uint32_t now = millis();

    switch (s.phase) {

        case Phase::WaitRelease:
            // Stay here until all buttons are physically released
            if (!raw) s.phase = Phase::Idle;
            break;

        case Phase::Idle:
            if (raw) {
                s.phase  = Phase::Debounce;
                s.downAt = now;
            }
            break;

        case Phase::Debounce:
            if (!raw) {
                // Released before debounce window = mechanical bounce, discard
                s.phase = Phase::Idle;
            } else if (now - s.downAt >= Config::Timing::DEBOUNCE_MS) {
                s.phase = Phase::Pressed;
            }
            break;

        case Phase::Pressed:
            if (!raw) {
                s.phase = Phase::Idle;
                if (pin == Config::Pins::BTN_LEFT)   return ButtonEvent::Left;
                if (pin == Config::Pins::BTN_CENTER) return ButtonEvent::Center;
                if (pin == Config::Pins::BTN_RIGHT)  return ButtonEvent::Right;
            } else if (now - s.downAt >= Config::Timing::LONG_PRESS_MS) {
                s.phase = Phase::LongHeld;
                if (pin == Config::Pins::BTN_CENTER) return ButtonEvent::LongCenter;
            }
            break;

        case Phase::LongHeld:
            if (!raw) s.phase = Phase::Idle;
            break;
    }
    return ButtonEvent::None;
}
