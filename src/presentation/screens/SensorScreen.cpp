#include "SensorScreen.h"

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
        d.drawText(0, 0, "Environment  [1/2]");
        d.drawLine(0, 9, d.width() - 1, 9);
        snprintf(buf, sizeof(buf), "Temp  : %7.2f C",    data.temperature);  d.drawText(0, 12, buf);
        snprintf(buf, sizeof(buf), "Humid : %7.1f %%",   data.humidity);     d.drawText(0, 22, buf);
        snprintf(buf, sizeof(buf), "Press : %7.1f hPa",  data.pressure);     d.drawText(0, 32, buf);
        snprintf(buf, sizeof(buf), "Sound : %7.1f dB",   data.soundDb);      d.drawText(0, 42, buf);
    } else {
        d.drawText(0, 0, "Power & Net  [2/2]");
        d.drawLine(0, 9, d.width() - 1, 9);
        snprintf(buf, sizeof(buf), "Volt   : %6.2f V",   data.voltage);      d.drawText(0, 12, buf);
        snprintf(buf, sizeof(buf), "Battery: %6d %%",    data.batteryPct);   d.drawText(0, 22, buf);
        snprintf(buf, sizeof(buf), "RSSI   : %6d dBm",   (int)data.rssi);    d.drawText(0, 32, buf);
        snprintf(buf, sizeof(buf), "Signal : %6d %%",    data.signalPct);    d.drawText(0, 42, buf);
    }

    d.drawLine(0, 55, d.width() - 1, 55);
    d.drawText(0, 57, (_page == 0) ? "< Kembali    Next >" : "< Prev            >");
    d.flush();
}

void SensorScreen::onButton(ButtonEvent evt) {
    switch (evt) {
        case ButtonEvent::Left:
            if (_page == 0)
                _done = true;  // Left at page 1 = back to menu
            else
                _page--;
            break;
        case ButtonEvent::Right:
            _page = min(1, _page + 1);
            break;
        default:
            break;
    }
}
