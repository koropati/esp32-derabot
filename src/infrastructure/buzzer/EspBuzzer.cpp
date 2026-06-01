#include "EspBuzzer.h"
#include "../../config/config.h"
#include <Arduino.h>

static constexpr int LEDC_RES = 8;   // 8-bit resolution

bool EspBuzzer::begin() {
    _pin     = Config::Pins::BUZZER;
    _channel = 0;
    pinMode(_pin, OUTPUT);
    ledcSetup(_channel, 1000, LEDC_RES);
    ledcAttachPin(_pin, _channel);
    ledcWrite(_channel, 0);  // silent
    return true;
}

void EspBuzzer::on(int freq) {
    ledcSetup(_channel, freq, LEDC_RES);
    ledcWrite(_channel, 128);  // 50% duty = audible square wave
    _on = true;
}

void EspBuzzer::beep(int freq, int durationMs) {
    on(freq);
    delay(durationMs);
    off();
}

void EspBuzzer::off() {
    ledcWrite(_channel, 0);
    _on = false;
}
