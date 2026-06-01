#include "BuzzerUseCase.h"

BuzzerUseCase::BuzzerUseCase(IBuzzer* buzzer, IStorage* storage)
    : _buzzer(buzzer), _storage(storage) {}

bool BuzzerUseCase::begin() {
    _storage->loadThreshold(_threshold);
    return _buzzer->begin();
}

// Returns true if last 5 voltage readings indicate charging (fluctuation pattern)
bool BuzzerUseCase::_detectCharging(float voltage) {
    // Add new reading to ring buffer
    _voltHist[_voltIdx] = voltage;
    _voltIdx = (_voltIdx + 1) % VOLT_HIST;
    if (_voltIdx == 0) _voltFull = true;

    if (!_voltFull) return false;  // not enough data yet

    float vmin = _voltHist[0], vmax = _voltHist[0], vsum = _voltHist[0];
    for (int i = 1; i < VOLT_HIST; i++) {
        if (_voltHist[i] < vmin) vmin = _voltHist[i];
        if (_voltHist[i] > vmax) vmax = _voltHist[i];
        vsum += _voltHist[i];
    }
    float vavg = vsum / VOLT_HIST;

    // Charging if: values fluctuate (range > 0.1V) AND
    //              average voltage is above min cutoff (real readings, not 0V)
    // Fluctuating low-voltage could also mean a bad battery — only suppress
    // volt alarm when average is in a reasonable range (> 3.0V)
    return (vmax - vmin > 0.1f) && (vavg > 3.0f);
}

void BuzzerUseCase::check(const SensorData& data) {
    if (!data.valid) return;
    if (_warmupCount < WARMUP_READS) { ++_warmupCount; return; }
    if (millis() < _snoozeUntilMs) return;

    if (!_threshold.enabled) {
        if (_triggered) silence();
        return;
    }

    bool charging = _detectCharging(data.voltage);

    // Suppress volt alarm when: threshold not set, reading too low to be real,
    // or charging is detected (fluctuating voltage)
    bool voltAlarm = !charging &&
                     (_threshold.voltMin > 0.1f) &&
                     (data.voltage > 0.5f) &&
                     (data.voltage < _threshold.voltMin);

    bool alarm = (data.temperature > _threshold.tempMax  ||
                  data.temperature < _threshold.tempMin  ||
                  data.humidity    > _threshold.humidMax ||
                  data.soundDb     > _threshold.soundMax ||
                  voltAlarm);

    if (alarm) {
        if (++_alarmCount >= ALARM_CONFIRM && !_triggered) {
            _buzzer->on(1000);
            _triggered = true;
        }
    } else {
        _alarmCount = 0;
        if (_triggered) silence();
    }
}

void BuzzerUseCase::silence() {
    _buzzer->off();
    _triggered     = false;
    _alarmCount    = 0;
    _snoozeUntilMs = millis() + SNOOZE_MS;
}

void BuzzerUseCase::click() {
    if (_triggered) return;
    _buzzer->on(2500);               // start immediately — no blocking
    _clickUntilMs = millis() + 20;  // tick() will stop it after 20ms
}

void BuzzerUseCase::tick() {
    if (_clickUntilMs && millis() >= _clickUntilMs) {
        _clickUntilMs = 0;
        if (!_triggered) _buzzer->off();  // don't cut alarm if it started
    }
}

void BuzzerUseCase::updateThreshold(const ThresholdConfig& cfg) {
    _threshold = cfg;
    _storage->saveThreshold(cfg);
}
