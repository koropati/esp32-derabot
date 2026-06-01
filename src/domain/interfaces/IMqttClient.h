#pragma once
#include <Arduino.h>
#include <functional>

using MqttCallback = std::function<void(const String& topic, const String& payload)>;

class IMqttClient {
public:
    virtual ~IMqttClient() = default;
    virtual bool begin() = 0;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool publish(const String& topic, const String& payload) = 0;
    virtual bool subscribe(const String& topic) = 0;
    virtual void setCallback(MqttCallback cb) = 0;
    virtual void loop() = 0;
};
