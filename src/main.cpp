#include <Arduino.h>
#include <Wire.h>

#include "config/config.h"

// Infrastructure
#include "infrastructure/sensors/Bme280Sensor.h"
#include "infrastructure/sensors/Inmp441Sensor.h"
#include "infrastructure/sensors/VoltSensor.h"
#include "infrastructure/sensors/Gy271Compass.h"
#include "infrastructure/display/OledDisplay.h"
#include "infrastructure/mqtt/HiveMqClient.h"
#include "infrastructure/wifi/EspWifiManager.h"
#include "infrastructure/storage/NvsStorage.h"
#include "infrastructure/buzzer/EspBuzzer.h"
#include "infrastructure/stock/YahooStockClient.h"

// Application
#include "application/SensorUseCase.h"
#include "application/WifiUseCase.h"
#include "application/MqttUseCase.h"
#include "application/BuzzerUseCase.h"
#include "application/StockUseCase.h"
#include "application/PowerUseCase.h"

// Presentation
#include "presentation/UIManager.h"

// ---------------------------------------------------------------------------
// Hardware instances (constructed once, never destroyed)
// ---------------------------------------------------------------------------
static OledDisplay    display;
static Bme280Sensor   bme280;
static Inmp441Sensor  inmp441;
static VoltSensor     voltSensor;
static Gy271Compass   compass;
static EspWifiManager wifiMgr;
static NvsStorage     storage;
static HiveMqClient     mqttClient;
static EspBuzzer        buzzer;
static YahooStockClient stockClient;

// ---------------------------------------------------------------------------
// Use cases
// ---------------------------------------------------------------------------
static SensorUseCase* sensorUC = nullptr;
static WifiUseCase*   wifiUC   = nullptr;
static MqttUseCase*   mqttUC   = nullptr;
static BuzzerUseCase* buzzerUC = nullptr;
static StockUseCase*  stockUC  = nullptr;
static PowerUseCase*  powerUC  = nullptr;

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------
static UIManager* ui = nullptr;

// ---------------------------------------------------------------------------
// Loop timers
// ---------------------------------------------------------------------------
static uint32_t lastSensorMs   = 0;
static uint32_t lastMqttPubMs  = 0;
static uint32_t nextMqttConnMs = 0;
static uint32_t mqttBackoffMs  = Config::Timing::MQTT_RETRY_MIN_MS;
static uint32_t lastDisplayMs  = 0;

// Motion-wake state: last magnetometer sample taken while the panel was dark.
static uint32_t lastMotionMs = 0;
static int16_t  mwX = 0, mwY = 0, mwZ = 0;
static bool     mwHave = false;

// ---------------------------------------------------------------------------

// Branded boot screen: framed title, subtitle, a status line and a progress bar
// that fills as init proceeds. `pct` 0..100 drives the bar.
static void bootScreen(const char* status, int pct) {
    const int W = Config::Display::W;
    const int H = Config::Display::H;
    display.clear();
    display.drawRect(0, 0, W, H, false);                 // outer frame
    display.drawText(22, 7, "DeraBot", 2);               // large title (centered)
    display.drawText(7, 28, "Environment Monitor");      // subtitle (centered)
    display.drawLine(8, 40, W - 8, 40);
    display.drawText(6, 44, status);

    const int bx = 8, by = 53, bw = W - 16, bh = 8;
    display.drawRect(bx, by, bw, bh, false);             // progress outline
    int fill = (bw - 4) * constrain(pct, 0, 100) / 100;
    if (fill > 0) display.drawRect(bx + 2, by + 2, fill, bh - 4, true);
    display.flush();
}

static void i2cScan() {
    Serial.println("[I2C] Scanning bus...");
    bool found = false;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C]  device at 0x%02X\n", addr);
            found = true;
        }
    }
    if (!found) Serial.println("[I2C]  no devices found - check wiring!");
}

void setup() {
    Serial.begin(115200);
    delay(500);  // wait for USB CDC to connect
    Serial.println("\n\n=== DeraBot v1.0 ===");

    // I2C shared bus for BME280 and OLED
    Wire.begin(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL);
    Wire.setClock(100000);  // 100kHz for stable I2C
    delay(100);             // let bus stabilize

    // Scan I2C for diagnosis
    i2cScan();

    // Persistent storage (NVS)
    storage.begin();

    // Display — must come early so splash works
    if (!display.begin()) {
        Serial.println("[OLED] FAILED - device will run without display");
    }

    bootScreen("Memulai sistem...", 10);
    delay(600);

    // Sensors
    bootScreen("Sensor...", 30);
    sensorUC = new SensorUseCase({ &bme280, &inmp441, &voltSensor, &compass });
    sensorUC->begin();

    // WiFi
    bootScreen("WiFi...", 50);
    wifiUC = new WifiUseCase(&wifiMgr, &storage);
    wifiUC->begin();

    // MQTT
    bootScreen("MQTT...", 65);
    mqttUC = new MqttUseCase(&mqttClient);
    mqttUC->begin();

    // Set MQTT incoming command handler
    mqttClient.setCallback([](const String& topic, const String& payload) {
        Serial.printf("[MQTT] <- %s : %s\n", topic.c_str(), payload.c_str());
        // Handle commands here (e.g. buzzer silence, threshold update)
        if (topic == Config::Mqtt::T_CMD && payload == "silence") {
            buzzerUC->silence();
        }
    });

    // Buzzer thresholds (loaded from NVS)
    bootScreen("Buzzer...", 75);
    buzzerUC = new BuzzerUseCase(&buzzer, &storage);
    buzzerUC->begin();
    buzzerUC->playStartup();  // short power-on blip

    // Stock (market code selectable in Settings, fetched via Yahoo Finance)
    stockUC = new StockUseCase(&stockClient, &storage);
    stockUC->begin();

    // Power-save manager (loads eco preference, applies CPU/WiFi state)
    powerUC = new PowerUseCase(&display, wifiUC, &storage);
    powerUC->begin();

    // UI
    bootScreen("Antarmuka...", 85);
    ui = new UIManager(&display, sensorUC, wifiUC, buzzerUC, stockUC, powerUC, &compass);
    ui->begin();

    // Try auto-connect to saved WiFi
    bootScreen("Menyambung WiFi...", 92);
    if (wifiUC->autoConnect()) {
        bootScreen(("Siap! " + wifiUC->getIp()).c_str(), 100);
        delay(1200);
    } else {
        bootScreen("Siap (tanpa WiFi)", 100);
        delay(1200);
    }
}

void loop() {
    // Buttons FIRST — before any blocking operation so no press is ever missed
    ui->pollButtons();

    uint32_t now = millis();

    // Motion wake — only while the panel is dark (eco screen-off) and enabled.
    // A large change in the magnetometer vector means the device was moved/rotated,
    // so wake the screen without needing a button press.
    if (powerUC && powerUC->motionWake() && !powerUC->screenOn()) {
        if (now - lastMotionMs >= Config::Power::MOTION_POLL_MS) {
            lastMotionMs = now;
            int16_t x, y, z;
            if (compass.readVector(x, y, z)) {
                if (mwHave) {
                    long d = labs(x - mwX) + labs(y - mwY) + labs(z - mwZ);
                    if (d >= Config::Power::MOTION_THRESHOLD) {
                        powerUC->wake();
                        ui->render();   // light up immediately
                    }
                }
                mwX = x; mwY = y; mwZ = z; mwHave = true;
            }
        }
    } else {
        mwHave = false;  // panel on (or disabled): re-baseline next time it sleeps
    }

    // Read all sensors on schedule
    if (now - lastSensorMs >= Config::Timing::SENSOR_MS) {
        lastSensorMs = now;
        sensorUC->read();

        // Battery % from the user-configurable gauge range (Settings: BatMin/BatMax)
        {
            ThresholdConfig th = buzzerUC->getThreshold();
            float span = th.battMax - th.battMin;
            int pct = (span > 0.05f)
                ? (int)((sensorUC->lastData().voltage - th.battMin) / span * 100.0f)
                : 0;
            sensorUC->setBatteryPct(constrain(pct, 0, 100));
        }

        // Enrich with live WiFi signal data
        if (wifiUC->isConnected()) {
            int32_t rssi = wifiUC->getRssi();
            int     pct  = constrain((int)map(rssi, -100, -50, 0, 100), 0, 100);
            sensorUC->updateWifiInfo(rssi, pct);
        }

        // Check thresholds and trigger buzzer if needed
        buzzerUC->check(sensorUC->lastData());
    }

    // MQTT — keep the session serviced, publish on schedule when connected.
    // The TLS connect is blocking (seconds when the broker is unreachable), so
    // it is throttled with exponential backoff: a failing broker can no longer
    // freeze the UI every MQTT_MS. Button input is unaffected regardless, since
    // it runs in its own task, but this keeps the display/MQTT loop responsive.
    if (wifiUC->isConnected()) {
        mqttUC->loop();
        if (mqttUC->isConnected()) {
            mqttBackoffMs = Config::Timing::MQTT_RETRY_MIN_MS;  // healthy — reset
            if (now - lastMqttPubMs >= Config::Timing::MQTT_MS) {
                lastMqttPubMs = now;
                mqttUC->publishSensor(sensorUC->lastData());
            }
        } else if (now >= nextMqttConnMs) {
            bool ok = mqttUC->ensureConnected();
            nextMqttConnMs = millis() + (ok ? Config::Timing::MQTT_RETRY_MIN_MS
                                            : mqttBackoffMs);
            if (!ok)
                mqttBackoffMs = min(mqttBackoffMs * 2,
                                    Config::Timing::MQTT_RETRY_MAX_MS);
        }
    }

    // Display render — throttle to DISPLAY_MS to avoid flooding I2C bus
    if (now - lastDisplayMs >= Config::Timing::DISPLAY_MS) {
        lastDisplayMs = now;
        ui->render();
    }
}
