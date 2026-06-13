#pragma once
#include "../../domain/interfaces/IDisplay.h"
#include "../../config/config.h"
#include <Adafruit_SSD1306.h>

class OledDisplay : public IDisplay {
public:
    OledDisplay() = default;
    ~OledDisplay();
    bool begin()  override;
    void clear()  override;
    void flush()  override;
    void drawText(int x, int y, const String& text, uint8_t size = 1) override;
    void drawLine(int x0, int y0, int x1, int y1) override;
    void drawRect(int x, int y, int w, int h, bool filled = false) override;
    void drawCircle(int x, int y, int r, bool filled = false) override;
    void drawRoundRect(int x, int y, int w, int h, int r, bool filled = false) override;
    void drawBitmap(int x, int y, const uint8_t* bmp, int w, int h) override;
    int  width()  const override { return _d ? _d->width()  : Config::Display::W; }
    int  height() const override { return _d ? _d->height() : Config::Display::H; }
    void setOn(bool on) override;
    void dim(bool dim)  override;
    bool isReady() const { return _d != nullptr; }

private:
    Adafruit_SSD1306* _d = nullptr;
};
