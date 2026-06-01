#pragma once

class IBuzzer {
public:
    virtual ~IBuzzer() = default;
    virtual bool begin() = 0;
    virtual void beep(int freq, int durationMs) = 0;
    virtual void on(int freq) = 0;
    virtual void off() = 0;
    virtual bool isOn() const = 0;
};
