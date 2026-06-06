#include "CompassScreen.h"
#include <Arduino.h>
#include <math.h>

static const char* cardinal(float h) {
    static const char* d[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return d[((int)((h + 22.5f) / 45.0f)) & 7];
}

// Approximate a circle with a 24-segment polygon (IDisplay has no circle prim).
static void drawRing(IDisplay& d, int cx, int cy, int r) {
    const int N = 24;
    float px = 0, py = 0;
    for (int i = 0; i <= N; i++) {
        float a = 2.0f * (float)PI * i / N;
        float x = cx + r * cosf(a);
        float y = cy + r * sinf(a);
        if (i > 0) d.drawLine((int)px, (int)py, (int)x, (int)y);
        px = x;
        py = y;
    }
}

CompassScreen::CompassScreen(Gy271Compass* compass) : _compass(compass) {}

void CompassScreen::enter() {
    _done       = false;
    _heading    = _compass ? _compass->readHeading() : -1.0f;
    _lastReadMs = millis();
}

void CompassScreen::tick() {
    // Refresh the heading at ~14 Hz so the needle feels live.
    if (!_compass) return;
    uint32_t now = millis();
    if (now - _lastReadMs < 70) return;
    _lastReadMs = now;
    float h = _compass->readHeading();
    if (h >= 0.0f) _heading = h;
}

void CompassScreen::update(IDisplay& d) {
    d.clear();

    if (_heading < 0.0f) {
        d.drawText(0, 0, "Compass");
        d.drawLine(0, 9, d.width() - 1, 9);
        d.drawText(0, 22, "GY-271 tidak");
        d.drawText(0, 32, "terdeteksi.");
        d.drawLine(0, 55, d.width() - 1, 55);
        d.drawText(0, 57, "< Kembali");
        d.flush();
        return;
    }

    const int cx = 31, cy = 33, r = 27;
    drawRing(d, cx, cy, r);

    // Cardinal letters just inside the ring.
    d.drawText(cx - 2,     cy - r + 1, "N");
    d.drawText(cx - 2,     cy + r - 8, "S");
    d.drawText(cx + r - 7, cy - 3,     "E");
    d.drawText(cx - r + 2, cy - 3,     "W");

    // Needle: tip toward heading (clockwise from North = up), short opposite tail.
    float rad = _heading * (float)PI / 180.0f;
    int tipX = cx + (int)((r - 5) * sinf(rad));
    int tipY = cy - (int)((r - 5) * cosf(rad));
    int tailX = cx - (int)(8 * sinf(rad));
    int tailY = cy + (int)(8 * cosf(rad));
    d.drawLine(cx, cy, tipX, tipY);
    d.drawLine(cx + 1, cy, tipX, tipY);   // double line = thicker needle
    d.drawLine(cx, cy, tailX, tailY);
    d.drawRect(tipX - 1, tipY - 1, 3, 3, true);  // arrow head dot

    // Right column: numeric heading + cardinal.
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)(_heading + 0.5f));
    d.drawText(70, 14, buf, 2);
    d.drawRect(70 + 12 * (int)strlen(buf) + 1, 14, 3, 3, false);  // degree mark
    d.drawText(70, 38, cardinal(_heading), 2);

    d.drawLine(0, 55, d.width() - 1, 55);
    d.drawText(70, 57, "< Kembali");
    d.flush();
}

void CompassScreen::onButton(ButtonEvent evt) {
    if (evt == ButtonEvent::Left) _done = true;
}
