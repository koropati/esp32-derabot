#pragma once
#include "../../domain/interfaces/IBuzzer.h"

class EspBuzzer : public IBuzzer {
public:
    bool begin()   override;
    void beep(int freq, int durationMs) override;
    void on(int freq, uint8_t duty = 128) override;
    void off()     override;
    bool isOn() const  override { return _on; }

private:
    int  _pin     = -1;
    int  _channel = 0;   // LEDC channel
    bool _on      = false;
};
