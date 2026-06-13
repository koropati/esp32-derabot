#pragma once
#include <Arduino.h>

class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual bool begin() = 0;
    virtual void clear() = 0;
    virtual void flush() = 0;
    virtual void drawText(int x, int y, const String& text, uint8_t size = 1) = 0;
    virtual void drawLine(int x0, int y0, int x1, int y1) = 0;
    virtual void drawRect(int x, int y, int w, int h, bool filled = false) = 0;
    virtual void drawCircle(int x, int y, int r, bool filled = false) = 0;
    virtual void drawRoundRect(int x, int y, int w, int h, int r, bool filled = false) = 0;
    virtual void drawBitmap(int x, int y, const uint8_t* bmp, int w, int h) = 0;
    virtual int  width()  const = 0;
    virtual int  height() const = 0;

    // Power control — default no-ops so non-OLED backends need not implement them.
    virtual void setOn(bool on) {}   // turn the panel on/off (power save)
    virtual void dim(bool dim)  {}   // reduce panel brightness/contrast
};
