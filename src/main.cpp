#include <Arduino.h>
#include <Wire.h>

#include "config/config.h"

// Infrastructure
#include "infrastructure/sensors/Bme280Sensor.h"
#include "infrastructure/sensors/Inmp441Sensor.h"
#include "infrastructure/sensors/VoltSensor.h"
#include "infrastructure/display/OledDisplay.h"
#include "infrastructure/mqtt/HiveMqClient.h"
#include "infrastructure/wifi/EspWifiManager.h"
#include "infrastructure/storage/NvsStorage.h"
#include "infrastructure/buzzer/EspBuzzer.h"

// Application
#include "application/SensorUseCase.h"
#include "application/WifiUseCase.h"
#include "application/MqttUseCase.h"
#include "application/BuzzerUseCase.h"

// Presentation
#include "presentation/UIManager.h"

// ---------------------------------------------------------------------------
// Hardware instances (constructed once, never destroyed)
// ---------------------------------------------------------------------------
static OledDisplay    display;
static Bme280Sensor   bme280;
static Inmp441Sensor  inmp441;
static VoltSensor     voltSensor;
static EspWifiManager wifiMgr;
static NvsStorage     storage;
static HiveMqClient   mqttClient;
static EspBuzzer      buzzer;

// ---------------------------------------------------------------------------
// Use cases
// ---------------------------------------------------------------------------
static SensorUseCase* sensorUC = nullptr;
static WifiUseCase*   wifiUC   = nullptr;
static MqttUseCase*   mqttUC   = nullptr;
static BuzzerUseCase* buzzerUC = nullptr;

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------
static UIManager* ui = nullptr;

// ---------------------------------------------------------------------------
// Loop timers
// ---------------------------------------------------------------------------
static uint32_t lastSensorMs  = 0;
static uint32_t lastMqttMs    = 0;
static uint32_t lastDisplayMs = 0;

// ---------------------------------------------------------------------------

static void showSplash(const char* msg) {
    display.clear();
    display.drawText(0, 16, " DeraBot v1.0");
    display.drawText(0, 30, msg);
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

    showSplash(" Initializing...");
    delay(800);

    // Sensors
    sensorUC = new SensorUseCase({ &bme280, &inmp441, &voltSensor });
    sensorUC->begin();

    // WiFi
    wifiUC = new WifiUseCase(&wifiMgr, &storage);
    wifiUC->begin();

    // MQTT
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
    buzzerUC = new BuzzerUseCase(&buzzer, &storage);
    buzzerUC->begin();

    // UI
    ui = new UIManager(&display, sensorUC, wifiUC, buzzerUC);
    ui->begin();

    // Try auto-connect to saved WiFi
    showSplash(" Connecting WiFi...");
    if (wifiUC->autoConnect()) {
        showSplash((" Got: " + wifiUC->getIp()).c_str());
        delay(1200);
    } else {
        showSplash(" No saved WiFi.");
        delay(1200);
    }
}

void loop() {
    // Buttons FIRST — before any blocking operation so no press is ever missed
    ui->pollButtons();

    uint32_t now = millis();

    // Read all sensors on schedule
    if (now - lastSensorMs >= Config::Timing::SENSOR_MS) {
        lastSensorMs = now;
        sensorUC->read();

        // Enrich with live WiFi signal data
        if (wifiUC->isConnected()) {
            int32_t rssi = wifiUC->getRssi();
            int     pct  = constrain((int)map(rssi, -100, -50, 0, 100), 0, 100);
            sensorUC->updateWifiInfo(rssi, pct);
        }

        // Check thresholds and trigger buzzer if needed
        buzzerUC->check(sensorUC->lastData());
    }

    // MQTT publish on schedule (only when connected)
    if (wifiUC->isConnected()) {
        mqttUC->loop();
        if (now - lastMqttMs >= Config::Timing::MQTT_MS) {
            lastMqttMs = now;
            if (mqttUC->ensureConnected()) {
                mqttUC->publishSensor(sensorUC->lastData());
            }
        }
    }

    // Display render — throttle to DISPLAY_MS to avoid flooding I2C bus
    if (now - lastDisplayMs >= Config::Timing::DISPLAY_MS) {
        lastDisplayMs = now;
        ui->render();
    }
}
