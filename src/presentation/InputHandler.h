#pragma once
#include "IScreen.h"
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Reads the three push-buttons inside a dedicated high-priority FreeRTOS task,
// so sampling is never disturbed by blocking work on the main loop (OLED I2C
// flush of tens of ms, or MQTT/WiFi connects of several seconds).
//
// Each press emits exactly ONE event, fired on PRESS-DOWN (right after the
// debounce window) so the action and its click feedback are instant. A line
// that is held / stuck LOW for too long is parked until it releases cleanly, so
// a faulty or noisy button can never flood the UI with repeated events. The
// task only does GPIO reads + millis() timing and posts events into a
// thread-safe queue; the UI thread drains it, so all screen-state mutation
// stays on one thread (no locks needed).
class InputHandler {
public:
    void        begin();
    ButtonEvent poll();      // non-blocking: pops one queued event (None if empty)
    void        resetAll();  // call after a screen transition — drops carry-over

private:
    enum class Phase : uint8_t {
        Idle,         // armed, waiting for a press
        Debounce,     // press seen, confirming it is real (not bounce)
        Held,         // event already fired, waiting for release
        WaitRelease,  // ignore this line until it is released and stable
    };

    struct BtnState {
        Phase    phase        = Phase::Idle;
        uint32_t downAt       = 0;     // when the current press/hold began
        uint32_t relAt        = 0;     // when the line was last seen released
        uint8_t  pressedLevel = LOW;   // GPIO level that means "pressed" (auto-detected)
    };

    BtnState      _left, _center, _right;
    QueueHandle_t _queue    = nullptr;
    TaskHandle_t  _task     = nullptr;
    volatile bool _resetReq = false;

    static void _taskTrampoline(void* arg);
    void        _run();  // task body — loops forever sampling the buttons
    ButtonEvent _processPin(int pin, BtnState& s);
    void        _neutralize(int pin, BtnState& s);
};
