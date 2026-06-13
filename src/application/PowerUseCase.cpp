#include "PowerUseCase.h"
#include "../config/config.h"
#include <Wire.h>

PowerUseCase::PowerUseCase(IDisplay* display, WifiUseCase* wifi, IStorage* storage)
    : _display(display), _wifi(wifi), _storage(storage) {}

void PowerUseCase::begin() {
    _storage->loadEco(_eco);
    _storage->loadMotionWake(_motionWake);
    _lastActivityMs = millis();
    // Apply the hardware state deterministically on boot.
    setCpuFrequencyMhz(_eco ? Config::Power::CPU_LO_MHZ : Config::Power::CPU_HI_MHZ);
    Wire.begin(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL);
    _wifi->setPowerSave(_eco);
    _display->setOn(true);
    _display->dim(false);
    _screenOn = true;
    _dimmed   = false;
}

void PowerUseCase::setEco(bool on) {
    if (on != _eco) _apply(on);
    _storage->saveEco(on);
    wake();   // show the effect immediately (and don't blank the screen the user
              // is looking at right after toggling)
}

void PowerUseCase::setMotionWake(bool on) {
    _motionWake = on;
    _storage->saveMotionWake(on);
}

void PowerUseCase::wake() {
    _lastActivityMs = millis();
    if (!_screenOn) { _display->setOn(true); _screenOn = true; }
    if (_dimmed)    { _display->dim(false);  _dimmed   = false; }
}

void PowerUseCase::holdAwake(bool on) {
    if (on == _hold) return;
    _hold = on;
    if (on) {
        setCpuFrequencyMhz(Config::Power::CPU_HI_MHZ);  // full speed for AP + web server
        Wire.begin(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL);
        wake();                                          // screen on + bright
    } else {
        // Back to whatever eco dictates; screen returns to the normal idle timers.
        setCpuFrequencyMhz(_eco ? Config::Power::CPU_LO_MHZ : Config::Power::CPU_HI_MHZ);
        Wire.begin(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL);
    }
}

void PowerUseCase::tick(bool keepAwake) {
    // Outside eco, or while something demands attention (alarm / awake-hold),
    // keep the panel lit.
    if (!_eco || keepAwake || _hold) {
        if (keepAwake || _hold) wake();
        return;
    }

    uint32_t idle = millis() - _lastActivityMs;
    if (idle >= Config::Power::SCREEN_OFF_MS) {
        if (_screenOn) { _display->setOn(false); _screenOn = false; }
    } else if (idle >= Config::Power::SCREEN_DIM_MS) {
        if (!_dimmed && _screenOn) { _display->dim(true); _dimmed = true; }
    }
}

void PowerUseCase::_apply(bool on) {
    _eco = on;
    setCpuFrequencyMhz(on ? Config::Power::CPU_LO_MHZ : Config::Power::CPU_HI_MHZ);
    Wire.begin(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL);
    _wifi->setPowerSave(on);
    if (!on) wake();   // leaving eco — restore a bright, awake screen
}
