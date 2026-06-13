#include "GameScreen.h"
#include <Arduino.h>

GameScreen::GameScreen(IStorage* storage, BuzzerUseCase* buzzer, PowerUseCase* power)
    : _storage(storage), _buzzer(buzzer), _power(power) {}

void GameScreen::enter() {
    _done = false;
    _state = GameState::Start;
    if (_storage) {
        _storage->loadHighScore(_highScore);
    }
    if (_power) {
        _power->holdAwake(true); // Prevent device sleep during play
    }
}

void GameScreen::exit() {
    if (_power) {
        _power->holdAwake(false); // Restore normal sleep timers
    }
}

void GameScreen::_resetGame() {
    _y = 24.0f;
    _vy = 0.0f;
    _score = 0;
    _spawnPipe(0, 128.0f);
    _spawnPipe(1, 128.0f + 70.0f); // Space pipes 70 pixels apart
}

void GameScreen::_spawnPipe(int idx, float startX) {
    _pipeX[idx] = startX;
    // Gap center Y ranges from 16 to 48 (safe margins)
    _pipeGapY[idx] = 16 + random(26);
}

void GameScreen::tick() {
    if (_state != GameState::Playing) return;
    
    uint32_t now = millis();
    // Update physics at 25 FPS (40ms)
    if (now - _lastFrameTime >= 40) {
        _lastFrameTime = now;
        
        // Apply gravity and update position
        _vy += GRAVITY;
        _y += _vy;
        
        // Dynamic speed difficulty ramp
        float speed = 2.0f;
        if (_score < 3) {
            speed = 1.2f;
        } else if (_score < 8) {
            speed = 1.6f;
        }
        
        // Scroll obstacles
        for (int i = 0; i < 2; i++) {
            float prevX = _pipeX[i];
            _pipeX[i] -= speed; // scroll speed
            
            // Score: when pipe successfully passes player x=30 (x=28 boundary check)
            if (prevX >= 28.0f && _pipeX[i] < 28.0f) { 
                _score++;
                if (_buzzer) _buzzer->click(); // play scoring blip
            }
            
            // Respawn pipe when it goes off screen
            if (_pipeX[i] < -PIPE_WIDTH) {
                _spawnPipe(i, 128.0f);
            }
        }
        
        // Boundary collision check
        if (_y > 60.0f || _y < -4.0f) {
            Serial.printf("[GameScreen] GameOver: out of bounds (y=%.2f, vy=%.2f)\n", _y, _vy);
            _state = GameState::GameOver;
            if (_score > _highScore) {
                _highScore = _score;
                if (_storage) _storage->saveHighScore(_highScore);
            }
            if (_buzzer) _buzzer->playMochiAngry(); // Sad crash beep
        }
        
        // Pipe collision check
        int px = 30; // Player left x
        int py = (int)_y; // Player top y
        int pw = 8; // Player width
        int ph = 8; // Player height
        
        for (int i = 0; i < 2; i++) {
            // Check if player bounding box overlaps pipe x span
            if (_pipeX[i] < px + pw && _pipeX[i] + PIPE_WIDTH > px) {
                int gapTop = _pipeGapY[i] - GAP_HEIGHT/2;
                int gapBottom = _pipeGapY[i] + GAP_HEIGHT/2;
                
                // If player is above gap top or below gap bottom -> collision!
                if (py < gapTop || py + ph > gapBottom) {
                    Serial.printf("[GameScreen] GameOver: collision pipe %d (pipeX=%.2f, py=%d, gap=%d..%d)\n", 
                                  i, _pipeX[i], py, gapTop, gapBottom);
                    _state = GameState::GameOver;
                    if (_score > _highScore) {
                        _highScore = _score;
                        if (_storage) _storage->saveHighScore(_highScore);
                    }
                    if (_buzzer) _buzzer->playMochiAngry(); // Sad crash beep
                }
            }
        }
    }
}

void GameScreen::update(IDisplay& d) {
    d.clear();

    if (_state == GameState::Start) {
        d.drawText(46, 4, "FLAPPY", 1);
        d.drawText(34, 15, "MOCHI", 2);
        d.drawText(34, 34, "Terbaik: " + String(_highScore));
        d.drawText(22, 44, "Lompat: Tengah");
        
        // Button hints aligned to physical buttons (Left, Center, Right)
        d.drawLine(0, 51, d.width() - 1, 51);
        d.drawText(4, 54, "Exit");
        d.drawText(46, 54, "Mulai");
        d.drawText(90, 54, "Mulai");
    } 
    else if (_state == GameState::Playing) {
        // 1. Draw Flappy Mochi (cute circle body with wing and eye)
        int py = (int)_y;
        d.drawCircle(34, py + 4, 4, true); // body
        
        // Flap wing based on velocity direction
        if (_vy < 0) {
            d.drawLine(28, py + 4, 30, py + 1); // Wing up
            d.drawLine(28, py + 4, 30, py + 7);
        } else {
            d.drawLine(28, py + 4, 30, py + 6); // Wing down
            d.drawLine(28, py + 4, 30, py + 2);
        }
        // Small eye (drawn as a tiny rect on white circle body)
        d.drawRect(35, py + 2, 1, 1, false); 
        
        // 2. Draw Obstacles (Pipes with lips)
        for (int i = 0; i < 2; i++) {
            if (_pipeX[i] >= -PIPE_WIDTH && _pipeX[i] < 128) {
                int gapTop = _pipeGapY[i] - GAP_HEIGHT/2;
                int gapBottom = _pipeGapY[i] + GAP_HEIGHT/2;
                
                // Top pipe
                d.drawRect(_pipeX[i], 0, PIPE_WIDTH, gapTop, true);
                // Top pipe lip (rim)
                d.drawRect(_pipeX[i] - 1, gapTop - 3, PIPE_WIDTH + 2, 3, true);
                
                // Bottom pipe
                d.drawRect(_pipeX[i], gapBottom, PIPE_WIDTH, 64 - gapBottom, true);
                // Bottom pipe lip (rim)
                d.drawRect(_pipeX[i] - 1, gapBottom, PIPE_WIDTH + 2, 3, true);
            }
        }
        
        // 3. Draw Score
        d.drawText(60, 2, String(_score), 2);
    } 
    else if (_state == GameState::GameOver) {
        d.drawText(10, 6, "GAME OVER", 2);
        d.drawText(40, 26, "Skor: " + String(_score));
        d.drawText(34, 38, "Terbaik: " + String(_highScore));
        
        // Button hints
        d.drawLine(0, 51, d.width() - 1, 51);
        d.drawText(4, 54, "Exit");
        d.drawText(46, 54, "Ulangi");
        d.drawText(88, 54, "Ulangi");
    }

    d.flush();
}

void GameScreen::onButton(ButtonEvent evt) {
    Serial.printf("[GameScreen] onButton: evt=%d, state=%d\n", (int)evt, (int)_state);
    if (_state == GameState::Start) {
        if (evt == ButtonEvent::Left) {
            _done = true; // Exit to menu
        } else if (evt == ButtonEvent::Center || evt == ButtonEvent::Right) {
            _state = GameState::Playing;
            _resetGame();
            _lastFrameTime = millis();
            if (_buzzer) _buzzer->playWifiTune(); // Rising happy jingle
        }
    } 
    else if (_state == GameState::Playing) {
        if (evt == ButtonEvent::Left) {
            _done = true; // Exit to menu
        } else if (evt == ButtonEvent::Center || evt == ButtonEvent::Right) {
            _vy = FLAP; // Flap wings
            if (_buzzer) _buzzer->click(); // play flap sound
        }
    } 
    else if (_state == GameState::GameOver) {
        if (evt == ButtonEvent::Left) {
            _done = true; // Exit to menu
        } else if (evt == ButtonEvent::Center || evt == ButtonEvent::Right) {
            _state = GameState::Playing;
            _resetGame();
            _lastFrameTime = millis();
            if (_buzzer) _buzzer->playWifiTune();
        }
    }
}
