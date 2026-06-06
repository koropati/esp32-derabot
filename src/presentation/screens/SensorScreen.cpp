#include "SensorScreen.h"

// 8-wind cardinal label for a heading in degrees (0=N, clockwise).
static const char* cardinal(float h) {
    static const char* d[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return d[((int)((h + 22.5f) / 45.0f)) & 7];
}

SensorScreen::SensorScreen(SensorUseCase* sensor) : _sensor(sensor) {}

void SensorScreen::enter() {
    _page = 0;
    _done = false;
}

void SensorScreen::update(IDisplay& d) {
    const SensorData& data = _sensor->lastData();
    d.clear();

    char buf[22];
    if (_page == 0) {
        d.drawText(0, 0, "Environment  [1/3]");
        d.drawLine(0, 9, d.width() - 1, 9);
        snprintf(buf, sizeof(buf), "Temp  : %7.2f C",    data.temperature);  d.drawText(0, 12, buf);
        snprintf(buf, sizeof(buf), "Humid : %7.1f %%",   data.humidity);     d.drawText(0, 22, buf);
        snprintf(buf, sizeof(buf), "Press : %7.1f hPa",  data.pressure);     d.drawText(0, 32, buf);
        snprintf(buf, sizeof(buf), "Sound : %7.1f dB",   data.soundDb);      d.drawText(0, 42, buf);
    } else if (_page == 1) {
        d.drawText(0, 0, "Power & Net  [2/3]");
        d.drawLine(0, 9, d.width() - 1, 9);
        snprintf(buf, sizeof(buf), "Volt   : %6.2f V",   data.voltage);      d.drawText(0, 12, buf);
        snprintf(buf, sizeof(buf), "Battery: %6d %%",    data.batteryPct);   d.drawText(0, 22, buf);
        snprintf(buf, sizeof(buf), "RSSI   : %6d dBm",   (int)data.rssi);    d.drawText(0, 32, buf);
        snprintf(buf, sizeof(buf), "Signal : %6d %%",    data.signalPct);    d.drawText(0, 42, buf);
    } else {
        d.drawText(0, 0, "Compass  [3/3]");
        d.drawLine(0, 9, d.width() - 1, 9);
        if (data.compassOk) {
            snprintf(buf, sizeof(buf), "Heading: %6.1f deg", data.heading);  d.drawText(0, 12, buf);
            snprintf(buf, sizeof(buf), "Arah   : %6s",       cardinal(data.heading)); d.drawText(0, 22, buf);
            d.drawText(0, 36, "GY-271 terhubung");
        } else {
            d.drawText(0, 16, "GY-271 tidak");
            d.drawText(0, 26, "terdeteksi.");
            d.drawText(0, 40, "Cek SDA/SCL & 3.3V");
        }
    }

    d.drawLine(0, 55, d.width() - 1, 55);
    if (_page == 0)
        d.drawText(0, 57, "< Kembali    Next >");
    else if (_page == 1)
        d.drawText(0, 57, "< Prev       Next >");
    else
        d.drawText(0, 57, "< Prev            ");
    d.flush();
}

void SensorScreen::onButton(ButtonEvent evt) {
    switch (evt) {
        case ButtonEvent::Left:
            if (_page == 0)
                _done = true;  // Left at first page = back to menu
            else
                _page--;
            break;
        case ButtonEvent::Right:
            _page = min(2, _page + 1);
            break;
        default:
            break;
    }
}
