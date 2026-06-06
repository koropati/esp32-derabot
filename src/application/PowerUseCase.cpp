#include "PowerUseCase.h"
#include "../config/config.h"

PowerUseCase::PowerUseCase(IDisplay* display, WifiUseCase* wifi, IStorage* storage)
    : _display(display), _wifi(wifi), _storage(storage) {}

void PowerUseCase::begin() {
    _storage->loadEco(_ecoManual);
    _lastActivityMs = millis();
    _active = _ecoManual;     // auto-low starts false; manual pref decides
    // Apply the hardware state deterministically on boot.
    setCpuFrequencyMhz(_active ? Config::Power::CPU_LO_MHZ : Config::Power::CPU_HI_MHZ);
    _wifi->setPowerSave(_active);
    _display->setOn(true);
    _display->dim(false);
    _screenOn = true;
    _dimmed   = false;
}

void PowerUseCase::setEco(bool on) {
    _ecoManual = on;
    _storage->saveEco(on);
    _recompute();
    wake();   // show the effect immediately (and don't blank the screen the user
              // is looking at right after toggling)
}

void PowerUseCase::onBattery(int pct) {
    // pct==0 happens before the first valid sensor read — ignore it so we don't
    // flip into eco on a phantom 0%.
    if (pct > 0) {
        if (pct <= Config::Power::LOW_BATT_PCT)      _autoLow = true;
        else if (pct >= Config::Power::RESUME_PCT)   _autoLow = false;
    }
    _recompute();
}

void PowerUseCase::wake() {
    _lastActivityMs = millis();
    if (!_screenOn) { _display->setOn(true); _screenOn = true; }
    if (_dimmed)    { _display->dim(false);  _dimmed   = false; }
}

void PowerUseCase::tick(bool keepAwake) {
    // Outside eco, or while something demands attention (alarm), keep it lit.
    if (!_active || keepAwake) {
        if (keepAwake) wake();
        return;
    }

    uint32_t idle = millis() - _lastActivityMs;
    if (idle >= Config::Power::SCREEN_OFF_MS) {
        if (_screenOn) { _display->setOn(false); _screenOn = false; }
    } else if (idle >= Config::Power::SCREEN_DIM_MS) {
        if (!_dimmed && _screenOn) { _display->dim(true); _dimmed = true; }
    }
}

void PowerUseCase::_recompute() {
    bool next = _ecoManual || _autoLow;
    if (next != _active) _setActive(next);
}

void PowerUseCase::_setActive(bool on) {
    _active = on;
    setCpuFrequencyMhz(on ? Config::Power::CPU_LO_MHZ : Config::Power::CPU_HI_MHZ);
    _wifi->setPowerSave(on);
    if (!on) wake();   // leaving eco — restore a bright, awake screen
}
