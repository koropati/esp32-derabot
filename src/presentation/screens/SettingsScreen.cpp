#include "SettingsScreen.h"

SettingsScreen::SettingsScreen(BuzzerUseCase* buzzer, PowerUseCase* power)
    : _buzzer(buzzer), _power(power) {}

void SettingsScreen::enter() {
    _cfg   = _buzzer->getThreshold();
    _eco   = _power->eco();
    _field = 0;
    _done  = false;
    // Two toggles first, then the value fields, then the action rows.
    _fields[0] = { "Alarm",   nullptr,        0,      0,       1    };
    _fields[1] = { "Hemat",   nullptr,        0,      0,       1    };
    _fields[2] = { "TempMax", &_cfg.tempMax,  1.0f,  -20.0f, 100.0f };
    _fields[3] = { "TempMin", &_cfg.tempMin,  1.0f,  -40.0f,  60.0f };
    _fields[4] = { "HumMaks", &_cfg.humidMax, 5.0f,   30.0f, 100.0f };
    _fields[5] = { "dBMaks",  &_cfg.soundMax, 5.0f,   30.0f, 130.0f };
    _fields[6] = { "VoltMin", &_cfg.voltMin,  0.1f,    2.5f,   4.2f };
    _fields[7] = { "BatMin",  &_cfg.battMin,  0.1f,    2.5f,   4.0f };
    _fields[8] = { "BatMax",  &_cfg.battMax,  0.1f,    3.0f,   4.3f };
    _fields[9]  = { "Simpan", nullptr,        0,      0,       0    };
    _fields[10] = { "Batal",  nullptr,        0,      0,       0    };
}

void SettingsScreen::update(IDisplay& d) {
    d.clear();
    d.drawText(0, 0, "Pengaturan");
    d.drawLine(0, 9, d.width() - 1, 9);

    // Show 4 fields at a time, scroll with cursor
    constexpr int VISIBLE = 4;
    int start = max(0, _field - VISIBLE + 1);
    for (int i = 0; i < VISIBLE; i++) {
        int idx = start + i;
        if (idx >= FIELD_COUNT) break;
        char buf[22];
        if (idx == ALARM_FIELD) {
            snprintf(buf, sizeof(buf), "%s%s: %s",
                (idx == _field) ? ">" : " ",
                _fields[idx].label,
                _cfg.enabled ? "AKTIF" : "NONAKTIF");
        } else if (idx == ECO_FIELD) {
            snprintf(buf, sizeof(buf), "%s%s: %s",
                (idx == _field) ? ">" : " ",
                _fields[idx].label,
                _eco ? "AKTIF" : "NONAKTIF");
        } else if (idx == SAVE_FIELD || idx == CANCEL_FIELD) {  // action rows
            snprintf(buf, sizeof(buf), "%s[ %s ]",
                (idx == _field) ? ">" : " ",
                _fields[idx].label);
        } else {
            snprintf(buf, sizeof(buf), "%s%s:%5.1f",
                (idx == _field) ? ">" : " ",
                _fields[idx].label,
                *_fields[idx].val);
        }
        d.drawText(0, 11 + i * 11, buf);
    }

    d.drawLine(0, 55, d.width() - 1, 55);
    if (_field == SAVE_FIELD || _field == CANCEL_FIELD)
        d.drawText(0, 57, "Ctr:Pilih  < >:Pindah");
    else
        d.drawText(0, 57, "< >:Atur  Ctr:Lanjut");
    d.flush();
}

void SettingsScreen::onButton(ButtonEvent evt) {
    Field& f = _fields[_field];
    switch (evt) {
        case ButtonEvent::Left:
            if (_field == ALARM_FIELD)      _cfg.enabled = false;
            else if (_field == ECO_FIELD)   _eco = false;
            else if (f.val)                       // value field: decrease
                *f.val = max(f.min, *f.val - f.step);
            else                                  // action row: step back a field
                _field = (_field - 1 + FIELD_COUNT) % FIELD_COUNT;
            break;
        case ButtonEvent::Right:
            if (_field == ALARM_FIELD)      _cfg.enabled = true;
            else if (_field == ECO_FIELD)   _eco = true;
            else if (f.val)                       // value field: increase
                *f.val = min(f.max, *f.val + f.step);
            else                                  // action row: step forward a field
                _field = (_field + 1) % FIELD_COUNT;
            break;
        case ButtonEvent::Center:
            if (_field == SAVE_FIELD) {
                _buzzer->updateThreshold(_cfg);   // save alarm thresholds
                _power->setEco(_eco);             // save power-save preference
                _done = true;
            } else if (_field == CANCEL_FIELD) {
                _done = true;                     // cancel without saving
            } else {
                _field = (_field + 1) % FIELD_COUNT;  // next field
            }
            break;
        default:
            break;
    }
}
