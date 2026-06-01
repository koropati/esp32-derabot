#pragma once
#include "../domain/interfaces/IBuzzer.h"
#include "../domain/interfaces/IStorage.h"
#include "../domain/entities/SensorData.h"
#include "../domain/entities/ThresholdConfig.h"
#include <Arduino.h>

class BuzzerUseCase {
public:
    BuzzerUseCase(IBuzzer* buzzer, IStorage* storage);
    bool begin();
    void check(const SensorData& data);
    void silence();
    void click();  // non-blocking click — starts beep, tick() stops it
    void tick();   // call every loop to manage click sound timer
    void updateThreshold(const ThresholdConfig& cfg);
    ThresholdConfig getThreshold() const { return _threshold; }
    bool isTriggered() const { return _triggered; }
    bool isSnoozed()   const { return millis() < _snoozeUntilMs; }

private:
    bool _detectCharging(float voltage);

    IBuzzer*        _buzzer;
    IStorage*       _storage;
    ThresholdConfig _threshold;
    bool            _triggered     = false;
    uint8_t         _warmupCount   = 0;
    uint8_t         _alarmCount    = 0;
    uint32_t        _snoozeUntilMs = 0;
    uint32_t        _clickUntilMs  = 0;  // non-blocking click end time

    // Ring buffer of last 5 voltage readings for charging detection
    static constexpr int VOLT_HIST = 7;
    float   _voltHist[VOLT_HIST] = {};
    uint8_t _voltIdx             = 0;
    bool    _voltFull            = false;

    static constexpr uint8_t  WARMUP_READS  = 5;
    static constexpr uint8_t  ALARM_CONFIRM = 3;
    static constexpr uint32_t SNOOZE_MS     = 30000;
};
