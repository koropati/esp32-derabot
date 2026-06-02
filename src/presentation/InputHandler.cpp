#include "InputHandler.h"
#include "../config/config.h"
#include <Arduino.h>

void InputHandler::begin() {
    pinMode(Config::Pins::BTN_LEFT,   INPUT_PULLUP);
    pinMode(Config::Pins::BTN_CENTER, INPUT_PULLUP);
    pinMode(Config::Pins::BTN_RIGHT,  INPUT_PULLUP);
    delay(5);  // let the pins settle before sampling

    // Auto-detect polarity. With nothing pressed at boot, whatever each line
    // reads is its "released" level — the opposite level means "pressed". This
    // supports both active-LOW buttons (to GND, idle reads HIGH via pull-up) and
    // active-HIGH buttons (to VCC with a board pull-down, idle reads LOW).
    auto calibrate = [](int pin, BtnState& s) {
        int idle        = digitalRead(pin);
        s.pressedLevel  = (idle == HIGH) ? LOW : HIGH;
        s.phase         = Phase::Idle;
        s.relAt         = millis();
    };
    calibrate(Config::Pins::BTN_LEFT,   _left);
    calibrate(Config::Pins::BTN_CENTER, _center);
    calibrate(Config::Pins::BTN_RIGHT,  _right);

    Serial.printf("[BTN] polarity  L(gpio%d)=%s  C(gpio%d)=%s  R(gpio%d)=%s\n",
                  Config::Pins::BTN_LEFT,   _left.pressedLevel   == HIGH ? "active-HIGH" : "active-LOW",
                  Config::Pins::BTN_CENTER, _center.pressedLevel == HIGH ? "active-HIGH" : "active-LOW",
                  Config::Pins::BTN_RIGHT,  _right.pressedLevel  == HIGH ? "active-HIGH" : "active-LOW");

    _queue = xQueueCreate(Config::Input::QUEUE_LEN, sizeof(uint8_t));

    // Priority above the Arduino loopTask (priority 1) so the scheduler preempts
    // any blocking main-loop work (display flush, MQTT/WiFi) to keep sampling.
    xTaskCreate(_taskTrampoline, "btnInput",
                Config::Input::TASK_STACK, this,
                Config::Input::TASK_PRIO, &_task);
}

void InputHandler::_taskTrampoline(void* arg) {
    static_cast<InputHandler*>(arg)->_run();
}

void InputHandler::_run() {
    const TickType_t period = pdMS_TO_TICKS(Config::Input::POLL_MS);
    TickType_t       last   = xTaskGetTickCount();

    struct { int pin; BtnState* st; } btns[] = {
        { Config::Pins::BTN_LEFT,   &_left   },
        { Config::Pins::BTN_CENTER, &_center },
        { Config::Pins::BTN_RIGHT,  &_right  },
    };

    for (;;) {
        // A screen transition asked us to forget any press in progress.
        if (_resetReq) {
            for (auto& b : btns) _neutralize(b.pin, *b.st);
            _resetReq = false;
        }

        // Advance every button's state machine each tick — none gets starved.
        for (auto& b : btns) {
            ButtonEvent e = _processPin(b.pin, *b.st);
            if (e != ButtonEvent::None) {
                uint8_t code = static_cast<uint8_t>(e);
                xQueueSend(_queue, &code, 0);  // drop if full — never block task
            }
        }

        vTaskDelayUntil(&last, period);
    }
}

ButtonEvent InputHandler::poll() {
    uint8_t code;
    if (_queue && xQueueReceive(_queue, &code, 0) == pdTRUE)
        return static_cast<ButtonEvent>(code);
    return ButtonEvent::None;
}

// Called after every screen transition. Asks the task to drop any in-progress
// press (so a held button does not immediately act on the new screen) and
// clears events already queued for the screen we are leaving.
void InputHandler::resetAll() {
    _resetReq = true;
    if (_queue) xQueueReset(_queue);
}

void InputHandler::_neutralize(int pin, BtnState& s) {
    if (s.phase == Phase::Idle) return;
    s.phase = Phase::WaitRelease;
    s.relAt = millis();
}

ButtonEvent InputHandler::_processPin(int pin, BtnState& s) {
    bool     raw = (digitalRead(pin) == s.pressedLevel);  // auto-detected polarity
    uint32_t now = millis();

    auto eventFor = [pin]() -> ButtonEvent {
        if (pin == Config::Pins::BTN_LEFT)   return ButtonEvent::Left;
        if (pin == Config::Pins::BTN_CENTER) return ButtonEvent::Center;
        return ButtonEvent::Right;
    };

    switch (s.phase) {

        case Phase::Idle:
            if (raw) {
                s.phase  = Phase::Debounce;
                s.downAt = now;
            }
            break;

        case Phase::Debounce:
            if (!raw) {
                s.phase = Phase::Idle;             // released early = bounce/glitch
            } else if (now - s.downAt >= Config::Timing::DEBOUNCE_MS) {
                s.phase  = Phase::Held;            // confirmed — fire once, on press-down
                s.downAt = now;
                return eventFor();
            }
            break;

        case Phase::Held:
            if (!raw) {
                s.phase = Phase::Idle;             // clean release → re-arm
            } else if (now - s.downAt >= Config::Timing::BTN_STUCK_MS) {
                s.phase = Phase::WaitRelease;      // held/stuck too long → park it
                s.relAt = now;
            }
            break;

        case Phase::WaitRelease:
            // Only re-arm after the line has been continuously released
            // (HIGH) for a full debounce window — rejects a stuck/noisy line.
            if (raw) s.relAt = now;
            else if (now - s.relAt >= Config::Timing::DEBOUNCE_MS) s.phase = Phase::Idle;
            break;
    }
    return ButtonEvent::None;
}
