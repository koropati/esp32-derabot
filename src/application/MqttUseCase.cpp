#include "MqttUseCase.h"
#include "../config/config.h"
#include <ArduinoJson.h>

MqttUseCase::MqttUseCase(IMqttClient* client) : _client(client) {}

bool MqttUseCase::begin() { return _client->begin(); }

bool MqttUseCase::ensureConnected() {
    if (_client->isConnected()) return true;
    return _client->connect();
}

bool MqttUseCase::publishSensor(const SensorData& data) {
    if (!_client->isConnected()) return false;
    JsonDocument doc;
    doc["temperature"] = data.temperature;
    doc["humidity"]    = data.humidity;
    doc["pressure"]    = data.pressure;
    doc["sound_db"]    = data.soundDb;
    doc["voltage"]     = data.voltage;
    doc["battery_pct"] = data.batteryPct;
    doc["rssi"]        = data.rssi;
    doc["signal_pct"]  = data.signalPct;
    doc["ts"]          = data.timestamp;
    String payload;
    serializeJson(doc, payload);
    return _client->publish(Config::Mqtt::T_SENSOR, payload);
}

bool MqttUseCase::publishStatus(const String& status) {
    return _client->publish(Config::Mqtt::T_STATUS, status);
}

void MqttUseCase::loop() { _client->loop(); }
bool MqttUseCase::isConnected() const { return _client->isConnected(); }
