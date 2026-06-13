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

// Happy: C6, E6, G6, C7
static const Note MOCHI_HAPPY[] = {
    {1047, 80}, {1319, 80}, {1568, 80}, {2093, 150}
};
static const int MOCHI_HAPPY_LEN = sizeof(MOCHI_HAPPY)/sizeof(Note);

// Angry: F#5 (short), F#5 (short), F5 (longer)
static const Note MOCHI_ANGRY[] = {
    {370, 60}, {0, 20}, {370, 60}, {0, 20}, {349, 150}
};
static const int MOCHI_ANGRY_LEN = sizeof(MOCHI_ANGRY)/sizeof(Note);

// Dizzy: alternating high/low sliding sound
static const Note MOCHI_DIZZY[] = {
    {880, 70}, {1174, 70}, {880, 70}, {1174, 70}, {880, 120}
};
static const int MOCHI_DIZZY_LEN = sizeof(MOCHI_DIZZY)/sizeof(Note);

// Sleepy: E5 (long), C5 (long)
static const Note MOCHI_SLEEPY[] = {
    {659, 200}, {523, 300}
};
static const int MOCHI_SLEEPY_LEN = sizeof(MOCHI_SLEEPY)/sizeof(Note);

// Power-on: short rising blip G5->C6. Replaces the long Nokia jingle at boot —
// quick and light on memory; the full melody is reserved for the alarm.
static const Note BOOT[] = {
    {784,90},{1047,150},
};
static const int BOOT_LEN = sizeof(BOOT) / sizeof(BOOT[0]);


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
    _buzzer->on(2500, _vol(128));    // start immediately — no blocking
    _clickUntilMs = millis() + 20;  // tick() will stop it after 20ms
}

void BuzzerUseCase::playStartup() {
    // Power-on: a short two-note rising blip, played once. Boot only — no
    // loop/tick() running yet, so play it blocking.
    for (int i = 0; i < BOOT_LEN; i++) {
        const Note& n = BOOT[i];
        if (n.freq == 0) _buzzer->off();
        else             _buzzer->on(n.freq, _vol(90));
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

void BuzzerUseCase::playMochiHappy() {
    if (_triggered) return;
    _startMelody(MOCHI_HAPPY, MOCHI_HAPPY_LEN, false, 80);
}

void BuzzerUseCase::playMochiAngry() {
    if (_triggered) return;
    _startMelody(MOCHI_ANGRY, MOCHI_ANGRY_LEN, false, 100);
}

void BuzzerUseCase::playMochiDizzy() {
    if (_triggered) return;
    _startMelody(MOCHI_DIZZY, MOCHI_DIZZY_LEN, false, 80);
}

void BuzzerUseCase::playMochiSleepy() {
    if (_triggered) return;
    _startMelody(MOCHI_SLEEPY, MOCHI_SLEEPY_LEN, false, 60);
}

// Short blocking beep at an explicit volume. The normal click() uses the *saved*
// volume, so while the user is still adjusting the Volume slider in Settings this
// lets them hear the new level immediately (before it is saved).
void BuzzerUseCase::testBeep(float volumePercent) {
    if (_triggered || _melody) return;            // never cut the alarm/jingle
    long d = (long)128 * (long)volumePercent / 100;
    _buzzer->on(2500, (uint8_t)constrain(d, 0L, 255L));
    delay(60);
    _buzzer->off();
    _clickUntilMs = 0;                            // we already turned it off
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
            else             _buzzer->on(n.freq, _vol(_melodyDuty));
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
