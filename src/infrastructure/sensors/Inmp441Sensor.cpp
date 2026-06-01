#include "Inmp441Sensor.h"
#include "../../config/config.h"
#include <Arduino.h>
#include <cmath>

bool Inmp441Sensor::begin() {
    i2s_config_t cfg = {};
    cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate          = Config::I2s::SAMPLE_RATE;
    cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;  // L/R pin to GND = left channel
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count        = Config::I2s::BUF_COUNT;
    cfg.dma_buf_len          = Config::I2s::BUF_LEN;
    cfg.use_apll             = false;

    i2s_pin_config_t pins = {};
    pins.mck_io_num   = I2S_PIN_NO_CHANGE;
    pins.bck_io_num   = Config::Pins::MIC_SCK;
    pins.ws_io_num    = Config::Pins::MIC_WS;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    pins.data_in_num  = Config::Pins::MIC_SD;

    esp_err_t r1 = i2s_driver_install((i2s_port_t)Config::I2s::PORT, &cfg, 0, nullptr);
    esp_err_t r2 = i2s_set_pin((i2s_port_t)Config::I2s::PORT, &pins);
    _ready = (r1 == ESP_OK && r2 == ESP_OK);
    if (!_ready) {
        Serial.printf("[INMP441] init failed r1=%d r2=%d\n", r1, r2);
    }
    return _ready;
}

bool Inmp441Sensor::read(SensorData& out) {
    if (!_ready) return false;
    int32_t buf[Config::I2s::BUF_LEN];
    size_t  bytesRead = 0;
    // pdMS_TO_TICKS(0) = non-blocking. DMA fills at 16kHz so data is always ready after 2s idle.
    i2s_read((i2s_port_t)Config::I2s::PORT, buf, sizeof(buf),
             &bytesRead, pdMS_TO_TICKS(0));
    size_t samples = bytesRead / sizeof(int32_t);
    if (samples == 0) return false;
    out.soundDb = computeDb(buf, samples);
    return true;
}

float Inmp441Sensor::computeDb(const int32_t* buf, size_t count) {
    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        // INMP441 delivers 24-bit data in the upper 24 bits of 32-bit word
        double s = static_cast<double>(buf[i] >> 8) / 8388608.0;
        sum += s * s;
    }
    double rms = sqrt(sum / count);
    if (rms < 1e-10) return 0.0f;
    // +94 dB offset to align 1 Pa (94 dBSPL) with INMP441 sensitivity (-26 dBFS)
    return static_cast<float>(20.0 * log10(rms) + 120.0);
}
