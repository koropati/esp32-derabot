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
// Eco is the OR of two sources: a manual toggle (persisted in NVS) and an
// automatic low-battery trigger (with hysteresis). The screen-blanking only
// applies while eco is active; otherwise the display stays fully on.
class PowerUseCase {
public:
    PowerUseCase(IDisplay* display, WifiUseCase* wifi, IStorage* storage);

    void begin();                 // load eco pref, apply hardware state
    void setEco(bool on);         // manual toggle from Settings (persists)
    bool eco()    const { return _ecoManual; }  // manual flag (Settings display)
    bool active() const { return _active; }     // effective eco state (ECO badge)

    void onBattery(int pct);      // auto low-battery activation (hysteresis)
    void wake();                  // user/alarm activity: screen on + bright, reset idle
    void tick(bool keepAwake);    // call every loop: handle dim/off timers
    bool screenOn() const { return _screenOn; }

private:
    void _recompute();            // active = manual || autoLow
    void _setActive(bool on);     // apply CPU/WiFi when effective state flips

    IDisplay*    _display;
    WifiUseCase* _wifi;
    IStorage*    _storage;

    bool _ecoManual = false;      // persisted manual preference
    bool _autoLow   = false;      // auto low-battery trigger
    bool _active    = false;      // effective eco state

    bool     _screenOn      = true;
    bool     _dimmed        = false;
    uint32_t _lastActivityMs = 0;
};
