#pragma once
#include "../../domain/interfaces/IMqttClient.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

class HiveMqClient : public IMqttClient {
public:
    bool begin()      override;
    bool connect()    override;
    void disconnect() override;
    bool isConnected() const override;
    bool publish(const String& topic, const String& payload) override;
    bool subscribe(const String& topic) override;
    void setCallback(MqttCallback cb)  override;
    void loop()       override;

private:
    WiFiClientSecure        _wifiClient;
    mutable PubSubClient    _mqtt{_wifiClient};
    MqttCallback     _cb;

    static void          _onMessage(char* topic, byte* payload, unsigned int len);
    static HiveMqClient* _instance;
};
