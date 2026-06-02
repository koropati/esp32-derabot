#pragma once
#include <cstdint>

class IBuzzer {
public:
    virtual ~IBuzzer() = default;
    virtual bool begin() = 0;
    virtual void beep(int freq, int durationMs) = 0;
    virtual void on(int freq, uint8_t duty = 128) = 0;  // duty 0-255 (volume/harshness)
    virtual void off() = 0;
    virtual bool isOn() const = 0;
};
