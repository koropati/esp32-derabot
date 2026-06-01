#pragma once
#include "../domain/interfaces/IMqttClient.h"
#include "../domain/entities/SensorData.h"

class MqttUseCase {
public:
    explicit MqttUseCase(IMqttClient* client);
    bool begin();
    bool ensureConnected();
    bool publishSensor(const SensorData& data);
    bool publishStatus(const String& status);
    void loop();
    bool isConnected() const;

private:
    IMqttClient* _client;
};
