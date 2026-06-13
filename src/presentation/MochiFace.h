#pragma once
#include "../domain/interfaces/IDisplay.h"
#include "../domain/entities/SensorData.h"
#include "../application/BuzzerUseCase.h"
#include "../application/PowerUseCase.h"
#include "../infrastructure/sensors/Gy271Compass.h"

enum class MochiState {
    Idle,
    Happy,
    Angry,
    Dizzy,
    Sleepy,
    Sleeping,
    Surprised,
    Cold,
    Hot,
    Noisy
};

class MochiFace {
public:
    MochiFace(BuzzerUseCase* buzzer, PowerUseCase* power, Gy271Compass* compass);
    void begin();
    
    // Updates face logic (timer, sensor inputs, state transitions)
    void tick(const SensorData& sensorData, bool anyButtonPressed, bool wifiConnected, bool enableShake = true);
    
    // Draws the current face on the display
    void draw(IDisplay& d);
    
    MochiState getState() const { return _state; }
    void setState(MochiState state);
    void wakeUp();

private:
    void _drawEye(IDisplay& d, int cx, int cy, bool isLeft);
    void _drawMouth(IDisplay& d, int cx, int cy);
    void _drawZzz(IDisplay& d);
    
    MochiState _state = MochiState::Idle;
    BuzzerUseCase* _buzzer = nullptr;
    PowerUseCase* _power = nullptr;
    Gy271Compass* _compass = nullptr;
    
    // Animation timers & offsets
    uint32_t _lastStateTime = 0;
    uint32_t _lastBlinkTime = 0;
    uint32_t _lastLookTime = 0;
    uint32_t _lastZzzTime = 0;
    
    bool _isBlinking = false;
    uint32_t _blinkStart = 0;
    
    int _lookX = 0;
    int _lookY = 0;
    int _breathOffset = 0;
    uint32_t _lastBreathTime = 0;
    bool _breathUp = true;
    
    // Zzz animation positions
    int _zzzX[3] = {85, 100, 115};
    int _zzzY[3] = {25, 18, 10};
    int _zzzState[3] = {0, 1, 2}; // scale / stage
    
    // Low battery / High temp state indicators
    bool _lowBattery = false;
    bool _highTemp = false;
    bool _loudNoise = false;
    
    // Input/State tracking
    uint32_t _lastInactivityMs = 0;
    
    int _signalPct = 0;
    int _batteryPct = 0;
    bool _wifiConnected = false;
    int _sleepBubbleStep = 0;
    
    // Shake detection variables
    uint32_t _lastShakePollMs = 0;
    int16_t _prevX = 0;
    int16_t _prevY = 0;
    int16_t _prevZ = 0;
    bool _hasPrevVector = false;
    uint32_t _lastShakeTime = 0;
};
