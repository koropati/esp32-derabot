#include "HiveMqClient.h"
#include "../../config/config.h"

HiveMqClient* HiveMqClient::_instance = nullptr;

bool HiveMqClient::begin() {
    _instance = this;
    // For production: replace setInsecure() with the HiveMQ root CA cert
    // _wifiClient.setCACert(hivemq_root_ca);
    _wifiClient.setInsecure();
    _mqtt.setServer(Config::Mqtt::HOST, Config::Mqtt::PORT);
    _mqtt.setBufferSize(Config::Mqtt::BUF_SIZE);
    _mqtt.setKeepAlive(60);
    _mqtt.setCallback(_onMessage);
    return true;
}

bool HiveMqClient::connect() {
    if (_mqtt.connected()) return true;
    bool ok = _mqtt.connect(Config::Mqtt::CLIENT,
                            Config::Mqtt::USER,
                            Config::Mqtt::PASS);
    if (ok) {
        _mqtt.subscribe(Config::Mqtt::T_CMD);
        Serial.println("[MQTT] connected");
    } else {
        Serial.printf("[MQTT] connect failed state=%d\n", _mqtt.state());
    }
    return ok;
}

void HiveMqClient::disconnect() { _mqtt.disconnect(); }
bool HiveMqClient::isConnected() const { return _mqtt.connected(); }

bool HiveMqClient::publish(const String& topic, const String& payload) {
    return _mqtt.publish(topic.c_str(), payload.c_str(), false);
}

bool HiveMqClient::subscribe(const String& topic) {
    return _mqtt.subscribe(topic.c_str());
}

void HiveMqClient::setCallback(MqttCallback cb) { _cb = cb; }
void HiveMqClient::loop() { _mqtt.loop(); }

void HiveMqClient::_onMessage(char* topic, byte* payload, unsigned int len) {
    if (!_instance || !_instance->_cb) return;
    String t(topic);
    String p;
    p.reserve(len);
    for (unsigned int i = 0; i < len; i++) p += static_cast<char>(payload[i]);
    _instance->_cb(t, p);
}
