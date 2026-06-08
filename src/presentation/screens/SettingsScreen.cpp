#include "SettingsScreen.h"
#include "../../config/config.h"

SettingsScreen::SettingsScreen(BuzzerUseCase* buzzer, PowerUseCase* power, StockUseCase* stock)
    : _buzzer(buzzer), _power(power), _stock(stock) {}

void SettingsScreen::enter() {
    _cfg        = _buzzer->getThreshold();
    _eco        = _power->eco();
    _motionWake = _power->motionWake();
    _stockIdx   = _stock->symbolIndex();
    _field      = 0;
    _done       = false;
    // Toggles + market selector first, then the value fields, then action rows.
    _fields[0]  = { "Alarm",     nullptr,        0,      0,       1    };
    _fields[1]  = { "Hemat",     nullptr,        0,      0,       1    };
    _fields[2]  = { "WakeGerak", nullptr,        0,      0,       1    };
    _fields[3]  = { "Saham",     nullptr,        0,      0,       0    };
    _fields[4]  = { "TempMax",   &_cfg.tempMax,  1.0f,  -20.0f, 100.0f };
    _fields[5]  = { "TempMin",   &_cfg.tempMin,  1.0f,  -40.0f,  60.0f };
    _fields[6]  = { "HumMaks",   &_cfg.humidMax, 5.0f,   30.0f, 100.0f };
    _fields[7]  = { "dBMaks",    &_cfg.soundMax, 5.0f,   30.0f, 130.0f };
    _fields[8]  = { "VoltMin",   &_cfg.voltMin,  0.1f,    0.0f,   4.2f };  // 0.0 = nonaktif
    _fields[9]  = { "BatMin",    &_cfg.battMin,  0.1f,    2.5f,   4.0f };
    _fields[10] = { "BatMax",    &_cfg.battMax,  0.1f,    3.0f,   4.3f };
    _fields[11] = { "Volume",    &_cfg.volume,   5.0f,    0.0f, 100.0f };
    _fields[12] = { "Simpan",    nullptr,        0,      0,       0    };
    _fields[13] = { "Batal",     nullptr,        0,      0,       0    };
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
        } else if (idx == WAKE_FIELD) {
            snprintf(buf, sizeof(buf), "%s%s:%s",
                (idx == _field) ? ">" : " ",
                _fields[idx].label,
                _motionWake ? "AKTIF" : "NONAKT");
        } else if (idx == SAHAM_FIELD) {
            snprintf(buf, sizeof(buf), "%s%s: %s",
                (idx == _field) ? ">" : " ",
                _fields[idx].label,
                Config::Stock::LIST[_stockIdx].label);
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
            else if (_field == WAKE_FIELD)  _motionWake = false;
            else if (_field == SAHAM_FIELD)       // selector: previous code (wraps)
                _stockIdx = (_stockIdx - 1 + Config::Stock::LIST_COUNT) % Config::Stock::LIST_COUNT;
            else if (f.val)                       // value field: decrease
                *f.val = max(f.min, *f.val - f.step);
            else                                  // action row: step back a field
                _field = (_field - 1 + FIELD_COUNT) % FIELD_COUNT;
            break;
        case ButtonEvent::Right:
            if (_field == ALARM_FIELD)      _cfg.enabled = true;
            else if (_field == ECO_FIELD)   _eco = true;
            else if (_field == WAKE_FIELD)  _motionWake = true;
            else if (_field == SAHAM_FIELD)       // selector: next code (wraps)
                _stockIdx = (_stockIdx + 1) % Config::Stock::LIST_COUNT;
            else if (f.val)                       // value field: increase
                *f.val = min(f.max, *f.val + f.step);
            else                                  // action row: step forward a field
                _field = (_field + 1) % FIELD_COUNT;
            break;
        case ButtonEvent::Center:  // (Volume preview handled after the switch)
            if (_field == SAVE_FIELD) {
                _buzzer->updateThreshold(_cfg);       // save alarm thresholds
                _power->setEco(_eco);                 // save power-save preference
                _power->setMotionWake(_motionWake);   // save motion-wake preference
                _stock->setSymbolIndex(_stockIdx);    // save market code selection
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

    // Live volume preview: when the Volume slider changes, beep at the new level
    // straight away so it can be heard before saving (the normal click feedback
    // still uses the previously saved volume).
    if (_field == VOLUME_FIELD && (evt == ButtonEvent::Left || evt == ButtonEvent::Right))
        _buzzer->testBeep(_cfg.volume);
}
