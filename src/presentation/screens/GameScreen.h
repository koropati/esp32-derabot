#pragma once
#include "../IScreen.h"
#include "../../domain/interfaces/IStorage.h"
#include "../../application/BuzzerUseCase.h"
#include "../../application/PowerUseCase.h"

enum class GameState {
    Start,
    Playing,
    GameOver
};

class GameScreen : public IScreen {
public:
    GameScreen(IStorage* storage, BuzzerUseCase* buzzer, PowerUseCase* power);
    void enter() override;
    void exit() override;
    void tick() override;
    void update(IDisplay& display) override;
    void onButton(ButtonEvent evt) override;
    bool isDone() const override { return _done; }

private:
    void _resetGame();
    void _spawnPipe(int idx, float startX);
    
    IStorage* _storage;
    BuzzerUseCase* _buzzer;
    PowerUseCase* _power;
    
    GameState _state = GameState::Start;
    bool _done = false;
    
    // Game variables
    float _y = 32.0f;
    float _vy = 0.0f;
    int _score = 0;
    int _highScore = 0;
    
    // Physics constants
    const float GRAVITY = 0.15f;
    const float FLAP = -2.5f;
    
    // Obstacle variables (2 pipes)
    float _pipeX[2] = {128.0f, 128.0f + 70.0f};
    int _pipeGapY[2] = {32, 32};
    const int PIPE_WIDTH = 8;
    const int GAP_HEIGHT = 22;
    
    // Frame timer
    uint32_t _lastFrameTime = 0;
};
