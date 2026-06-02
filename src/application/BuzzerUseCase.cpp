#include "BuzzerUseCase.h"

// ---------------------------------------------------------------------------
// Melodies (note frequencies in Hz, durations in ms). freq 0 = rest.
// ---------------------------------------------------------------------------

// Nokia Tune ("Gran Vals") — used as the alarm, looped while triggered.
static const Note NOKIA[] = {
    {659,150},{587,150},{370,300},{415,300},   // E5 D5 F#4 G#4
    {554,150},{494,150},{294,300},{330,300},   // C#5 B4 D4 E4
    {494,150},{440,150},{277,300},{330,300},   // B4 A4 C#4 E4
    {440,600},{0,450},                         // A4 (long) + rest before repeat
};
static const int NOKIA_LEN = sizeof(NOKIA) / sizeof(NOKIA[0]);

// WiFi setup: soft ascending major arpeggio C5-E5-G5-C6 ("welcome / open").
static const Note WIFI_TUNE[] = {
    {523,110},{659,110},{784,110},{1047,260},{0,40},
};
static const int WIFI_TUNE_LEN = sizeof(WIFI_TUNE) / sizeof(WIFI_TUNE[0]);

// IHSG load: soft descending motif E6-C6-G5 ("data chime") — distinct from WiFi.
static const Note STOCK_TUNE[] = {
    {1319,110},{1047,110},{784,240},{0,40},
};
static const int STOCK_TUNE_LEN = sizeof(STOCK_TUNE) / sizeof(STOCK_TUNE[0]);


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
            _triggered = true;
            _startMelody(NOKIA, NOKIA_LEN, /*loop=*/true, /*duty=*/128);  // Nokia Tune alarm
        }
    } else {
        _alarmCount = 0;
        if (_triggered) silence();
    }
}

void BuzzerUseCase::silence() {
    _melody        = nullptr;        // stop any alarm melody
    _buzzer->off();
    _triggered     = false;
    _alarmCount    = 0;
    _snoozeUntilMs = millis() + SNOOZE_MS;
}

void BuzzerUseCase::click() {
    if (_triggered || _melody) return;  // don't interrupt the alarm/melody
    _buzzer->on(2500);               // start immediately — no blocking
    _clickUntilMs = millis() + 20;  // tick() will stop it after 20ms
}

void BuzzerUseCase::playStartup() {
    // Power-on: the iconic Nokia Tune (Gran Vals), played once. Boot only — no
    // loop/tick() running yet, so play it blocking. (Skip the trailing rest.)
    for (int i = 0; i < NOKIA_LEN - 1; i++) {   // -1: drop the trailing loop rest
        const Note& n = NOKIA[i];
        if (n.freq == 0) _buzzer->off();
        else             _buzzer->on(n.freq, 110);
        delay(n.ms);
    }
    _buzzer->off();
}

void BuzzerUseCase::stopClick() {
    if (_triggered || _melody) return;  // never cut the alarm/melody
    _clickUntilMs = 0;
    _buzzer->off();
}

void BuzzerUseCase::playWifiTune() {
    if (_triggered) return;          // alarm has priority
    _startMelody(WIFI_TUNE, WIFI_TUNE_LEN, /*loop=*/false, /*duty=*/70);
}

void BuzzerUseCase::playStockTune() {
    if (_triggered) return;
    _startMelody(STOCK_TUNE, STOCK_TUNE_LEN, /*loop=*/false, /*duty=*/70);
}

void BuzzerUseCase::_startMelody(const Note* m, int len, bool loop, uint8_t duty) {
    _melody      = m;
    _melodyLen   = len;
    _melodyIdx   = 0;
    _melodyLoop  = loop;
    _melodyDuty  = duty;
    _clickUntilMs = 0;               // a melody overrides a pending click
    _noteUntilMs = millis();         // first note plays on the next tick
}

void BuzzerUseCase::tick() {
    uint32_t now = millis();

    // Melody playback owns the buzzer while active (alarm loop or UI jingle).
    if (_melody) {
        if (now >= _noteUntilMs) {
            if (_melodyIdx >= _melodyLen) {
                if (_melodyLoop) {
                    _melodyIdx = 0;          // repeat (alarm)
                } else {
                    _melody = nullptr;       // one-shot finished
                    _buzzer->off();
                    return;
                }
            }
            const Note& n = _melody[_melodyIdx++];
            if (n.freq == 0) _buzzer->off();
            else             _buzzer->on(n.freq, _melodyDuty);
            _noteUntilMs = now + n.ms;
        }
        return;
    }

    // Otherwise manage the short click feedback timer.
    if (_clickUntilMs && now >= _clickUntilMs) {
        _clickUntilMs = 0;
        if (!_triggered) _buzzer->off();
    }
}

void BuzzerUseCase::updateThreshold(const ThresholdConfig& cfg) {
    _threshold = cfg;
    _storage->saveThreshold(cfg);
}
