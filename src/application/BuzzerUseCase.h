#pragma once
#include "../domain/interfaces/IBuzzer.h"
#include "../domain/interfaces/IStorage.h"
#include "../domain/entities/SensorData.h"
#include "../domain/entities/ThresholdConfig.h"
#include <Arduino.h>

// A single tone in a melody. freq == 0 means a rest.
struct Note {
    uint16_t freq;
    uint16_t ms;
};

class BuzzerUseCase {
public:
    BuzzerUseCase(IBuzzer* buzzer, IStorage* storage);
    bool begin();
    void check(const SensorData& data);
    void silence();
    void click();          // non-blocking click — starts beep, tick() stops it
    void stopClick();      // silence a pending click NOW (before a blocking call)
    void tick();           // call every loop: advances melody / click timers
    void playStartup();    // short power-on blip (blocking, boot only)
    void playWifiTune();   // gentle ascending jingle (WiFi setup portal)
    void playStockTune();  // gentle descending jingle (loading IHSG data)
    void updateThreshold(const ThresholdConfig& cfg);
    ThresholdConfig getThreshold() const { return _threshold; }
    bool isTriggered() const { return _triggered; }
    bool isSnoozed()   const { return millis() < _snoozeUntilMs; }

private:
    bool _detectCharging(float voltage);
    void _startMelody(const Note* m, int len, bool loop, uint8_t duty);

    // Scale a sound's base PWM duty (0-255) by the master volume setting (0-100%).
    // 100% keeps the original loudness; 0% mutes. Applied to every buzzer tone so
    // clicks, the alarm, and all jingles follow one shared volume.
    uint8_t _vol(uint8_t baseDuty) const {
        long d = (long)baseDuty * (long)_threshold.volume / 100;
        return (uint8_t)constrain(d, 0L, 255L);
    }

    IBuzzer*        _buzzer;
    IStorage*       _storage;
    ThresholdConfig _threshold;
    bool            _triggered     = false;
    uint8_t         _warmupCount   = 0;
    uint8_t         _alarmCount    = 0;
    uint32_t        _snoozeUntilMs = 0;
    uint32_t        _clickUntilMs  = 0;  // non-blocking click end time

    // Non-blocking melody player
    const Note*     _melody       = nullptr;
    int             _melodyLen    = 0;
    int             _melodyIdx    = 0;
    bool            _melodyLoop   = false;
    uint8_t         _melodyDuty   = 128;
    uint32_t        _noteUntilMs  = 0;

    // Ring buffer of last 5 voltage readings for charging detection
    static constexpr int VOLT_HIST = 7;
    float   _voltHist[VOLT_HIST] = {};
    uint8_t _voltIdx             = 0;
    bool    _voltFull            = false;

    static constexpr uint8_t  WARMUP_READS  = 5;
    static constexpr uint8_t  ALARM_CONFIRM = 3;
    static constexpr uint32_t SNOOZE_MS     = 30000;
};
