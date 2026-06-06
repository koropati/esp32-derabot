#pragma once
#include "../domain/interfaces/IDisplay.h"

// Shared modal "loading" window: a centered bordered box with a title bar and a
// status line. Light on memory — no buffers, just primitive draw calls.
//
// It does NOT flush: the caller flushes once the frame is composed, so a blocking
// call (WiFi scan, HTTP fetch) can run right after the window is already on the
// OLED. That way the user sees "loading" during the freeze instead of a stale or
// blank screen.
inline void drawLoadingWindow(IDisplay& d, const String& title, const String& msg) {
    const int w  = d.width();
    const int h  = d.height();
    const int bx = 4;
    const int by = 10;
    const int bw = w - 8;   // window border width
    const int bh = h - 20;  // window border height

    d.clear();
    d.drawRect(bx, by, bw, bh);                          // window frame
    d.drawText(bx + 5, by + 5, title);                  // title
    d.drawLine(bx + 3, by + 16, bx + bw - 3, by + 16);  // title separator
    d.drawText(bx + 5, by + 24, msg);                   // status line
    d.drawText(bx + 5, by + 33, "[ Mohon tunggu ]");    // hint
}
