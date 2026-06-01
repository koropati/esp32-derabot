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

namespace Voltage {
    constexpr float VREF  = 3.3f;
    constexpr float R1    = 100000.0f;  // voltage divider upper resistor (Ohm)
    constexpr float R2    = 100000.0f;  // voltage divider lower resistor (Ohm)
    constexpr float V_MAX = 4.2f;       // LiPo full
    constexpr float V_MIN = 3.0f;       // LiPo cutoff
    constexpr int   ADC_MAX = 4095;
}

namespace Timing {
    constexpr uint32_t SENSOR_MS    = 2000;
    constexpr uint32_t MQTT_MS      = 5000;
    constexpr uint32_t DISPLAY_MS   = 100;
    constexpr uint32_t DEBOUNCE_MS   = 15;    // debounce confirm window (filters bounce)
    constexpr uint32_t LONG_PRESS_MS = 2500; // long press threshold — 2.5s is clearly intentional
}

namespace I2s {
    constexpr int PORT        = 0;
    constexpr int SAMPLE_RATE = 16000;
    constexpr int BUF_COUNT   = 4;
    constexpr int BUF_LEN     = 256;
}

} // namespace Config
