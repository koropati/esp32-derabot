#pragma once
#include <cstdint>
#include "config_secrets.h"  // credentials — git-ignored, see config_secrets.example.h

namespace Config {

namespace Pins {
    constexpr int MIC_WS    = 3;
    constexpr int MIC_SCK   = 4;
    constexpr int MIC_SD    = 2;
    constexpr int I2C_SDA   = 8;
    constexpr int I2C_SCL   = 9;
    constexpr int VOLT       = 0;
    constexpr int BUZZER     = 5;
    constexpr int BTN_LEFT   = 6;
    constexpr int BTN_CENTER = 7;
    constexpr int BTN_RIGHT  = 10;
}

namespace I2cAddr {
    constexpr uint8_t BME280  = 0x76;
    constexpr uint8_t OLED    = 0x3C;
    constexpr uint8_t QMC5883 = 0x0D;  // GY-271 (QMC5883L variant)
    constexpr uint8_t HMC5883 = 0x1E;  // GY-271 (HMC5883L variant)
}

namespace Compass {
    // Magnetic declination for your location (degrees, East positive). Leave 0
    // for raw magnetic north; set per-city for true north. e.g. Jakarta ~ +0.5.
    constexpr float DECLINATION = 0.0f;
}

namespace Mqtt {
    // Credentials sourced from config_secrets.h (git-ignored)
    constexpr const char* HOST     = Secrets::Mqtt::HOST;
    constexpr int         PORT     = Secrets::Mqtt::PORT;
    constexpr const char* USER     = Secrets::Mqtt::USER;
    constexpr const char* PASS     = Secrets::Mqtt::PASS;
    constexpr const char* CLIENT   = Secrets::Mqtt::CLIENT;
    // Topics — safe to publish
    constexpr const char* T_SENSOR = "derabot/sensor";
    constexpr const char* T_STATUS = "derabot/status";
    constexpr const char* T_CMD    = "derabot/command";
    constexpr int         BUF_SIZE = 2048;
}

namespace Display {
    constexpr int W = 128;
    constexpr int H = 64;
}

namespace Ap {
    // Soft-AP config portal: phone/laptop connects here to set WiFi via a web page.
    // PASS protects the hotspot so randoms can't open the config page. WPA2 needs
    // at least 8 characters; set "" for an open network. The password is shown on
    // the device screen so you know what to type on your phone.
    constexpr const char* SSID = "DeraBot-Setup";
    constexpr const char* PASS = "derabot123";   // >= 8 chars (WPA2); "" = open
}

namespace Stock {
    // Intraday market data from Yahoo Finance's chart endpoint.
    constexpr const char* HOST       = "query1.finance.yahoo.com";
    constexpr uint32_t    REFRESH_MS = 60000;  // auto-refresh while screen is open
    constexpr int         MAX_POINTS = 120;    // downsample graph to <= screen width

    // Selectable market codes — pick one in Settings ("Saham"). `label` is shown
    // on the device; `symbol` is the Yahoo Finance ticker put in the API request.
    // Add/remove rows freely; the first row is the default on a fresh flash.
    struct Symbol { const char* label; const char* symbol; };
    constexpr Symbol LIST[] = {
        { "IHSG",   "^JKSE"   },  // Jakarta Composite Index
        { "BBCA",   "BBCA.JK" },
        { "BBRI",   "BBRI.JK" },
        { "BMRI",   "BMRI.JK" },
        { "TLKM",   "TLKM.JK" },
        { "ASII",   "ASII.JK" },
        { "ANTM",   "ANTM.JK" },
        { "GOTO",   "GOTO.JK" },
        { "S&P 500","^GSPC"   },
        { "Nasdaq", "^IXIC"   },
    };
    constexpr int LIST_COUNT = sizeof(LIST) / sizeof(LIST[0]);
}

namespace Forex {
    // Rupiah (IDR) exchange rates from exchangerate-api's free open endpoint
    // (no API key required). Base USD; each listed currency is converted to IDR.
    constexpr const char* HOST       = "open.er-api.com";
    constexpr const char* PATH       = "/v6/latest/USD";
    constexpr uint32_t    REFRESH_MS = 1800000;   // 30 min (source updates ~daily)

    // Currencies shown as "1 <CODE> = ? IDR". Add/remove freely; "USD" must be
    // present for the conversion math (it's the API base).
    constexpr const char* CODES[]   = { "USD", "EUR", "SGD", "JPY", "MYR", "SAR" };
    constexpr int         CODES_COUNT = sizeof(CODES) / sizeof(CODES[0]);
}

namespace Voltage {
    constexpr float VREF  = 3.3f;
    constexpr float R1    = 100000.0f;  // voltage divider upper resistor (Ohm)
    constexpr float R2    = 100000.0f;  // voltage divider lower resistor (Ohm)
    constexpr float V_MAX = 3.7f;       // 100% — baterai 3.7V dianggap penuh
    constexpr float V_MIN = 3.2f;       // 0% — di bawah ini dianggap habis
    constexpr int   ADC_MAX = 4095;

    // Calibration multiplier applied to the final battery voltage. Corrects for
    // resistor tolerance + ADC error. Set CAL = (true battery V via multimeter)
    // / (voltage the firmware reports). Calibrated: pin 0.807V <-> battery 3.7V.
    constexpr float CAL     = 2.29f;
    constexpr int   SAMPLES = 16;       // ADC samples averaged per reading
    constexpr bool  DEBUG   = false;    // set true to print ADC mV for re-calibration
}

namespace Power {
    // Eco / power-save mode. When active: CPU clock is lowered, WiFi radio uses
    // modem-sleep, and the OLED dims then turns off after inactivity.
    constexpr uint32_t CPU_HI_MHZ = 160;   // normal CPU clock
    constexpr uint32_t CPU_LO_MHZ = 80;    // eco CPU clock

    // Auto-activate eco when battery drops to LOW_BATT_PCT, auto-release once it
    // recovers past RESUME_PCT (hysteresis avoids flapping near the threshold).
    constexpr int LOW_BATT_PCT = 20;
    constexpr int RESUME_PCT   = 35;

    // OLED inactivity timers (only while eco is active). Dim first, then off.
    constexpr uint32_t SCREEN_DIM_MS = 15000;
    constexpr uint32_t SCREEN_OFF_MS = 30000;

    // Motion wake (toggle "WakeGerak" in Settings): while the panel is dark in
    // eco, picking up / rotating the device changes the magnetometer vector enough
    // to wake the screen without a button press. POLL_MS = how often we sample the
    // sensor; THRESHOLD = Manhattan delta in raw LSB that counts as movement
    // (higher = needs a bigger move, fewer false wakes).
    constexpr uint32_t MOTION_POLL_MS   = 150;
    constexpr int      MOTION_THRESHOLD = 500;
}

namespace Timing {
    constexpr uint32_t SENSOR_MS    = 2000;
    constexpr uint32_t MQTT_MS      = 5000;
    constexpr uint32_t DISPLAY_MS   = 100;
    constexpr uint32_t DEBOUNCE_MS = 25;    // sustained-LOW window to confirm a real press
    // A line that stays LOW longer than this is treated as stuck / noisy: it
    // stops emitting and waits for a clean release, so a faulty button can never
    // storm the UI with repeated events.
    constexpr uint32_t BTN_STUCK_MS = 700;

    // MQTT reconnect backoff — a failing broker must NOT block the UI every
    // MQTT_MS with a multi-second TLS handshake. Retry interval starts at MIN
    // and doubles up to MAX while connect keeps failing.
    constexpr uint32_t MQTT_RETRY_MIN_MS = 5000;
    constexpr uint32_t MQTT_RETRY_MAX_MS = 60000;
}

namespace Input {
    // Buttons run in their own high-priority FreeRTOS task so debounce/long-press
    // timing is immune to blocking work on the main loop (OLED flush, MQTT/WiFi).
    constexpr uint32_t POLL_MS    = 5;     // button sampling period
    constexpr int      QUEUE_LEN  = 8;     // pending button events buffer
    constexpr uint32_t TASK_STACK = 2560;  // bytes — task only reads GPIO + millis
    constexpr int      TASK_PRIO  = 2;     // above Arduino loopTask (priority 1)
}

namespace I2s {
    constexpr int PORT        = 0;
    constexpr int SAMPLE_RATE = 16000;
    constexpr int BUF_COUNT   = 4;
    constexpr int BUF_LEN     = 256;
}

} // namespace Config
