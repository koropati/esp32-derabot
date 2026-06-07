#pragma once
#include "../domain/interfaces/IDisplay.h"
#include "../domain/interfaces/IStorage.h"
#include "WifiUseCase.h"
#include <Arduino.h>

// Owns the device power-save ("eco") state and applies its hardware effects:
//   - CPU clock lowered to Config::Power::CPU_LO_MHZ
//   - WiFi radio modem-sleep
//   - OLED dims then turns off after inactivity (wakes on button / alarm)
//
// Eco is controlled solely by the manual toggle in Settings (persisted in NVS),
// so turning it ON/OFF is fully deterministic. Screen-blanking only applies
// while eco is active; otherwise the display stays fully on and bright.
class PowerUseCase {
public:
    PowerUseCase(IDisplay* display, WifiUseCase* wifi, IStorage* storage);

    void begin();                 // load eco pref, apply hardware state
    void setEco(bool on);         // manual toggle from Settings (persists)
    bool eco()    const { return _eco; }   // manual flag (Settings display)
    bool active() const { return _eco; }   // effective eco state (ECO badge)

    void wake();                  // user/alarm activity: screen on + bright, reset idle
    void tick(bool keepAwake);    // call every loop: handle dim/off timers
    bool screenOn() const { return _screenOn; }

private:
    void _apply(bool on);         // apply CPU/WiFi for an eco state change

    IDisplay*    _display;
    WifiUseCase* _wifi;
    IStorage*    _storage;

    bool _eco = false;            // persisted manual preference == effective state

    bool     _screenOn      = true;
    bool     _dimmed        = false;
    uint32_t _lastActivityMs = 0;
};
