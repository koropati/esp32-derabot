#include "OledDisplay.h"
#include <Wire.h>
#include <Arduino.h>

OledDisplay::~OledDisplay() {
    delete _d;
}

bool OledDisplay::begin() {
    // Try 0x3C first (most common), then 0x3D (SA0 pulled high)
    const uint8_t addrs[] = { Config::I2cAddr::OLED, 0x3D };

    for (uint8_t addr : addrs) {
        auto* d = new Adafruit_SSD1306(Config::Display::W, Config::Display::H, &Wire, -1);
        if (d->begin(SSD1306_SWITCHCAPVCC, addr)) {
            delete _d;
            _d = d;
            _d->setTextColor(SSD1306_WHITE);
            _d->cp437(true);
            _d->clearDisplay();
            _d->display();
            Serial.printf("[OLED] OK at 0x%02X\n", addr);
            return true;
        }
        delete d;
        delay(10);
    }
    _d = nullptr;
    Serial.println("[OLED] FAILED - check SDA/SCL wiring");
    return false;
}

void OledDisplay::clear() {
    if (_d) _d->clearDisplay();
}

void OledDisplay::flush() {
    if (_d) _d->display();
}

void OledDisplay::drawText(int x, int y, const String& text, uint8_t size) {
    if (!_d) return;
    _d->setTextSize(size);
    _d->setTextColor(SSD1306_WHITE);
    _d->setCursor(x, y);
    _d->print(text);
}

void OledDisplay::drawLine(int x0, int y0, int x1, int y1) {
    if (_d) _d->drawLine(x0, y0, x1, y1, SSD1306_WHITE);
}

void OledDisplay::drawRect(int x, int y, int w, int h, bool filled) {
    if (!_d) return;
    if (filled)
        _d->fillRect(x, y, w, h, SSD1306_WHITE);
    else
        _d->drawRect(x, y, w, h, SSD1306_WHITE);
}

void OledDisplay::drawCircle(int x, int y, int r, bool filled) {
    if (!_d) return;
    if (filled)
        _d->fillCircle(x, y, r, SSD1306_WHITE);
    else
        _d->drawCircle(x, y, r, SSD1306_WHITE);
}

void OledDisplay::drawRoundRect(int x, int y, int w, int h, int r, bool filled) {
    if (!_d) return;
    if (filled)
        _d->fillRoundRect(x, y, w, h, r, SSD1306_WHITE);
    else
        _d->drawRoundRect(x, y, w, h, r, SSD1306_WHITE);
}

void OledDisplay::drawBitmap(int x, int y, const uint8_t* bmp, int w, int h) {
    if (_d) _d->drawBitmap(x, y, bmp, w, h, SSD1306_WHITE);
}

void OledDisplay::setOn(bool on) {
    // Sleep/wake the panel — the controller drops to ~uA when off (power save).
    if (_d) _d->ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void OledDisplay::dim(bool dim) {
    if (_d) _d->dim(dim);  // lowers contrast/segment current
}
