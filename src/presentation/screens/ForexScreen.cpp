#include "ForexScreen.h"
#include "../LoadingWindow.h"

ForexScreen::ForexScreen(ForexUseCase* forex, WifiUseCase* wifi, BuzzerUseCase* buzzer)
    : _forex(forex), _wifi(wifi), _buzzer(buzzer) {}

void ForexScreen::enter() {
    _done      = false;
    _needFetch = true;   // fetch as soon as the screen opens
    _announce  = true;   // chime once data has loaded
    _page      = 0;
}

void ForexScreen::tick() {
    // Schedule a refresh when due; the blocking fetch itself runs in update()
    // so a "loading" frame can be shown first.
    if (_wifi->isConnected() && _forex->due()) _needFetch = true;
}

// Round to whole rupiah and group thousands with dots (Indonesian style):
// 16250.4 -> "16.250".
static String idrStr(float v) {
    long n = (long)(v + 0.5f);
    String digits = String(n);
    String out;
    int cnt = 0;
    for (int i = digits.length() - 1; i >= 0; i--) {
        out = String(digits[i]) + out;
        if (++cnt % 3 == 0 && i > 0) out = "." + out;
    }
    return out;
}

void ForexScreen::update(IDisplay& d) {
    if (!_wifi->isConnected()) {
        d.clear();
        d.drawText(0, 0, "Kurs Rupiah");
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

        // Loading window for user-initiated loads (open / manual refresh) or when
        // there is no data yet. The silent timed auto-refresh updates in place.
        if (_announce || !_forex->data().valid) {
            drawLoadingWindow(d, "Kurs Rupiah", "Memuat data...");
            d.flush();
        }

        _forex->refresh();   // blocking (~1-3s); buttons stay live via input task
        _needFetch = false;
        if (_announce && _buzzer && _forex->data().valid) _buzzer->playStockTune();
        _announce = false;
    }

    const ExchangeData& s = _forex->data();
    if (!s.valid) {
        d.clear();
        d.drawText(0, 0, "Kurs Rupiah");
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

void ForexScreen::_drawData(IDisplay& d, const ExchangeData& s) {
    const int total = (int)s.rates.size();
    const int pages = (total + PER_PAGE - 1) / PER_PAGE;
    if (_page >= pages) _page = 0;                    // clamp after data changes
    const int first = _page * PER_PAGE;

    d.clear();

    // Header: title, plus a page indicator on the right when there's more than one.
    d.drawText(0, 0, "Kurs Rupiah");
    if (pages > 1) {
        char pg[8];
        snprintf(pg, sizeof(pg), "%d/%d", _page + 1, pages);
        d.drawText(d.width() - 6 * (int)strlen(pg), 0, pg);
    }
    d.drawLine(0, 11, d.width() - 1, 11);

    // Roomy rows: code on the left, "Rp <value>" right-aligned.
    int y = 16;
    for (int i = first; i < first + PER_PAGE && i < total; i++) {
        const auto& r = s.rates[i];
        d.drawText(0, y, r.code);
        String val = String("Rp ") + idrStr(r.idr);
        d.drawText(d.width() - 6 * (int)val.length(), y, val);
        y += 12;                                      // generous line spacing
    }

    d.drawLine(0, 54, d.width() - 1, 54);
    // Footer hints aligned to the three buttons (Right pages only when needed).
    d.drawText(0, 56, "<Kmbl");
    d.drawText(46, 56, "Muat");
    if (pages > 1)
        d.drawText(d.width() - 6 * 6, 56, "Lanj>");
    d.flush();
}

void ForexScreen::onButton(ButtonEvent evt) {
    if (evt == ButtonEvent::Left) {
        _done = true;                                  // back
    } else if (evt == ButtonEvent::Center) {
        _needFetch = true;                             // manual refresh
        _announce  = true;                             // chime after it loads
    } else if (evt == ButtonEvent::Right) {
        // Next page (wraps). Harmless when the list fits one screen.
        int total = (int)_forex->data().rates.size();
        int pages = (total + PER_PAGE - 1) / PER_PAGE;
        if (pages > 1) _page = (_page + 1) % pages;
    }
}
