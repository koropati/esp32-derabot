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
    constexpr uint8_t BME280 = 0x76;
    constexpr uint8_t OLED   = 0x3C;
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
    constexpr const char* SSID = "DeraBot-Setup";
    constexpr const char* PASS = "";   // empty = open network (easiest to join)
}

namespace Stock {
    // IHSG (Jakarta Composite) intraday from Yahoo Finance (symbol ^JKSE).
    constexpr const char* HOST       = "query1.finance.yahoo.com";
    constexpr const char* PATH       = "/v8/finance/chart/%5EJKSE?range=1d&interval=5m";
    constexpr uint32_t    REFRESH_MS = 60000;  // auto-refresh while screen is open
    constexpr int         MAX_POINTS = 120;    // downsample graph to <= screen width
}

namespace Voltage {
    constexpr float VREF  = 3.3f;
    constexpr float R1    = 100000.0f;  // voltage divider upper resistor (Ohm)
    constexpr float R2    = 100000.0f;  // voltage divider lower resistor (Ohm)
    constexpr float V_MAX = 4.2f;       // 100% (LiPo full)
    constexpr float V_MIN = 3.3f;       // 0% (safe cutoff)
    constexpr int   ADC_MAX = 4095;

    // Calibration multiplier applied to the final battery voltage. Corrects for
    // resistor tolerance + ADC error. Set CAL = (true battery V via multimeter)
    // / (voltage the firmware reports). Calibrated: pin 0.807V <-> battery 3.7V.
    constexpr float CAL     = 2.29f;
    constexpr int   SAMPLES = 16;       // ADC samples averaged per reading
    constexpr bool  DEBUG   = false;    // set true to print ADC mV for re-calibration
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
