#include "StockScreen.h"
#include <math.h>

StockScreen::StockScreen(StockUseCase* stock, WifiUseCase* wifi, BuzzerUseCase* buzzer)
    : _stock(stock), _wifi(wifi), _buzzer(buzzer) {}

void StockScreen::enter() {
    _done      = false;
    _needFetch = true;   // fetch as soon as the screen opens
    _announce  = true;   // chime once data has loaded
}

void StockScreen::tick() {
    // Schedule a refresh when due; the blocking fetch itself runs in update()
    // so a "loading" frame can be shown first.
    if (_wifi->isConnected() && _stock->due()) _needFetch = true;
}

void StockScreen::update(IDisplay& d) {
    if (!_wifi->isConnected()) {
        d.clear();
        d.drawText(0, 0, "Bursa IHSG");
        d.drawLine(0, 11, d.width() - 1, 11);
        d.drawText(0, 24, "Tidak ada internet.");
        d.drawText(0, 36, "Hubungkan WiFi dulu.");
        d.drawLine(0, 55, d.width() - 1, 55);
        d.drawText(0, 57, "< Kembali");
        d.flush();
        return;
    }

    if (_needFetch) {
        if (_buzzer) _buzzer->stopClick();   // kill the menu click before blocking

        // Only show a "loading" frame on the very first load (no data yet). Once
        // we already have data, keep the current value/graph on screen during the
        // (blocking) fetch and just refresh in place — so updates look seamless.
        if (!_stock->data().valid) {
            d.clear();
            d.drawText(0, 0, "Bursa IHSG");
            d.drawLine(0, 11, d.width() - 1, 11);
            d.drawText(10, 30, "Memuat data...");
            d.flush();
        }

        _stock->refresh();   // blocking (~1-3s); buttons stay live via input task
        _needFetch = false;
        // Chime AFTER the blocking fetch (so the melody isn't frozen), and only
        // for user-initiated loads — the silent 60s auto-refresh stays quiet.
        if (_announce && _buzzer && _stock->data().valid) _buzzer->playStockTune();
        _announce = false;
    }

    const StockData& s = _stock->data();
    if (!s.valid) {
        d.clear();
        d.drawText(0, 0, "Bursa IHSG");
        d.drawLine(0, 11, d.width() - 1, 11);
        d.drawText(0, 24, "Gagal memuat data:");
        d.drawText(0, 36, s.error.substring(0, 21));
        d.drawLine(0, 55, d.width() - 1, 55);
        d.drawText(0, 57, "< Kembali  Ctr:Ulang");
        d.flush();
        return;
    }

    _drawData(d, s);
}

void StockScreen::_drawData(IDisplay& d, const StockData& s) {
    char buf[26];
    d.clear();

    // Header: index value
    snprintf(buf, sizeof(buf), "IHSG %.2f", s.price);
    d.drawText(0, 0, buf);

    // Change line with up/down sign
    char sign = (s.change >= 0) ? '+' : '-';
    snprintf(buf, sizeof(buf), "%c%.2f (%c%.2f%%)",
             sign, fabsf(s.change), sign, fabsf(s.changePct));
    d.drawText(0, 10, buf);

    d.drawLine(0, 20, d.width() - 1, 20);

    _drawGraph(d, s, 23, 53);

    d.drawLine(0, 55, d.width() - 1, 55);
    d.drawText(0, 57, "< Kembali  Ctr:Muat");
    d.flush();
}

void StockScreen::_drawGraph(IDisplay& d, const StockData& s, int top, int bottom) {
    const auto& p = s.points;
    if (p.size() < 2) {
        d.drawText(12, 34, "(grafik kosong)");
        return;
    }

    float mn = p[0], mx = p[0];
    for (float v : p) { if (v < mn) mn = v; if (v > mx) mx = v; }
    if (s.prevClose > 0) { mn = min(mn, s.prevClose); mx = max(mx, s.prevClose); }
    if (mx - mn < 1e-3f) mx = mn + 1.0f;

    const int W = d.width();
    auto yOf = [&](float v) {
        return bottom - (int)((v - mn) / (mx - mn) * (bottom - top));
    };

    // Dotted baseline at previous close
    if (s.prevClose >= mn && s.prevClose <= mx) {
        int yb = yOf(s.prevClose);
        for (int x = 0; x < W; x += 4) d.drawRect(x, yb, 1, 1, true);
    }

    // Intraday curve
    int n = p.size();
    int prevX = 0, prevY = yOf(p[0]);
    for (int i = 1; i < n; i++) {
        int x = i * (W - 1) / (n - 1);
        int y = yOf(p[i]);
        d.drawLine(prevX, prevY, x, y);
        prevX = x;
        prevY = y;
    }
}

void StockScreen::onButton(ButtonEvent evt) {
    if (evt == ButtonEvent::Left) {
        _done = true;                                  // back
    } else if (evt == ButtonEvent::Center) {
        _needFetch = true;                             // manual refresh
        _announce  = true;                             // chime after it loads
    }
}
