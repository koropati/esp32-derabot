#include "MochiFace.h"
#include "../config/config.h"
#include <Arduino.h>

MochiFace::MochiFace(BuzzerUseCase* buzzer, PowerUseCase* power, Gy271Compass* compass)
    : _buzzer(buzzer), _power(power), _compass(compass) {}

void MochiFace::begin() {
    _lastStateTime = millis();
    _lastBlinkTime = millis();
    _lastLookTime = millis();
    _lastBreathTime = millis();
    _lastInactivityMs = millis();
    _lastShakePollMs = millis();
    _lastZzzTime = millis();
    _state = MochiState::Idle;
    _isBlinking = false;
    _lookX = 0;
    _lookY = 0;
    _breathOffset = 0;
    _hasPrevVector = false;
    _sleepBubbleStep = 0;
}

void MochiFace::setState(MochiState state) {
    if (_state == state) return;
    _state = state;
    _lastStateTime = millis();
    
    // Play sound matching the new emotion
    if (_buzzer) {
        switch (_state) {
            case MochiState::Happy:
                _buzzer->playMochiHappy();
                break;
            case MochiState::Angry:
                _buzzer->playMochiAngry();
                break;
            case MochiState::Dizzy:
                _buzzer->playMochiDizzy();
                break;
            case MochiState::Sleepy:
                _buzzer->playMochiSleepy();
                break;
            case MochiState::Sleeping:
                _buzzer->playMochiSleepy(); // Use sleepy tune for going to sleep
                break;
            case MochiState::Surprised:
                if (!_buzzer->isTriggered()) _buzzer->click(); // play a surprised blip
                break;
            case MochiState::Cold:
                _buzzer->playMochiDizzy(); // Shivering sound
                break;
            case MochiState::Hot:
                _buzzer->playMochiSleepy(); // Exhausted/sighing sound
                break;
            case MochiState::Noisy:
                _buzzer->playMochiAngry(); // Annoyed sound
                break;
            default:
                break;
        }
    }
}

void MochiFace::wakeUp() {
    if (_state == MochiState::Sleeping || _state == MochiState::Sleepy) {
        setState(MochiState::Idle);
        _lastInactivityMs = millis();
        if (_buzzer) _buzzer->playMochiHappy();
    }
}

void MochiFace::tick(const SensorData& sensorData, bool anyButtonPressed, bool wifiConnected, bool enableShake) {
    uint32_t now = millis();
    
    _wifiConnected = wifiConnected;
    if (sensorData.valid) {
        _batteryPct = sensorData.batteryPct;
        _signalPct = sensorData.signalPct;
        _lowBattery = (sensorData.batteryPct <= 20);
        _highTemp   = (sensorData.temperature >= 35.0f);
    } else {
        // defaults/fallback if invalid on fresh start
        _lowBattery = false;
        _highTemp   = false;
    }
    
    // 1. Reset inactivity if a button was pressed
    if (anyButtonPressed) {
        _lastInactivityMs = now;
        if (_state == MochiState::Sleeping || _state == MochiState::Sleepy) {
            wakeUp();
            return;
        }
    }
    
    // 2. Shake Detection via GY-271 Compass magnetometer vector
    bool shaken = false;
    if (enableShake && _compass && _compass->isReady()) {
        if (now - _lastShakePollMs >= 150) {
            _lastShakePollMs = now;
            int16_t x, y, z;
            if (_compass->readVector(x, y, z)) {
                if (_hasPrevVector) {
                    long diff = labs(x - _prevX) + labs(y - _prevY) + labs(z - _prevZ);
                    // Shaken threshold is 180 (Manhattan distance delta in raw LSB)
                    if (diff >= 180) {
                        shaken = true;
                        _lastShakeTime = now;
                        _lastInactivityMs = now; // reset inactivity on shake
                    }
                }
                _prevX = x;
                _prevY = y;
                _prevZ = z;
                _hasPrevVector = true;
            }
        }
    }
    
    // 3. Sensor integration (Noise, Temp) using configured thresholds
    bool isCold = false;
    bool isHot = false;
    bool isNoisy = false;
    
    if (sensorData.valid && _buzzer) {
        ThresholdConfig th = _buzzer->getThreshold();
        if (sensorData.temperature < th.tempMin) {
            isCold = true;
        } else if (sensorData.temperature > th.tempMax) {
            isHot = true;
        }
        if (sensorData.soundDb > th.soundMax) {
            isNoisy = true;
        }
    }
    
    // 4. Shaken reaction (Dizzy or Angry)
    if (shaken && _state != MochiState::Sleeping) {
        // Alternate between Angry and Dizzy randomly
        if (random(2) == 0) {
            setState(MochiState::Dizzy);
        } else {
            setState(MochiState::Angry);
        }
    } else if (now - _lastShakeTime > 4000 && (_state == MochiState::Dizzy || _state == MochiState::Angry)) {
        // Return to Idle after 4 seconds of no shake
        setState(MochiState::Idle);
    }
    
    // 5. Sensor reactions (Cold, Hot, Noisy) if not shaken and not sleeping/sleepy
    if (_state != MochiState::Sleeping && _state != MochiState::Sleepy && 
        _state != MochiState::Dizzy && _state != MochiState::Angry) {
        
        if (isNoisy) {
            setState(MochiState::Noisy);
            _lastInactivityMs = now; // loud noise wakes Mochi up and resets inactivity
        } else if (isHot) {
            setState(MochiState::Hot);
        } else if (isCold) {
            setState(MochiState::Cold);
        } else if (_state == MochiState::Noisy || _state == MochiState::Hot || _state == MochiState::Cold) {
            // Return to Idle if conditions are cleared
            setState(MochiState::Idle);
        }
    }
    
    // 6. Normal state machine and inactivity progression
    if (_state != MochiState::Sleeping && _state != MochiState::Dizzy && 
        _state != MochiState::Angry && _state != MochiState::Surprised) {
        
        uint32_t inactiveDuration = now - _lastInactivityMs;
        
        // Eco mode / Power save check
        bool ecoActive = _power && _power->active();
        
        if (ecoActive) {
            // Eco mode: goes to Sleepy 500ms before SCREEN_DIM_MS, Sleeping 500ms before SCREEN_OFF_MS
            if (inactiveDuration >= (Config::Power::SCREEN_OFF_MS - 500)) {
                setState(MochiState::Sleeping);
            } else if (inactiveDuration >= (Config::Power::SCREEN_DIM_MS - 500)) {
                setState(MochiState::Sleepy);
            } else {
                if (_state != MochiState::Cold && _state != MochiState::Hot && _state != MochiState::Noisy) {
                    setState(MochiState::Idle);
                }
            }
        } else {
            // Normal mode (eco disabled): goes to Sleepy after 20s, Sleeping after 45s (screen stays on)
            if (inactiveDuration >= 45000) {
                setState(MochiState::Sleeping);
            } else if (inactiveDuration >= 20000) {
                setState(MochiState::Sleepy);
            } else {
                if (_state != MochiState::Cold && _state != MochiState::Hot && _state != MochiState::Noisy) {
                    // Occasionally smile/laugh in idle
                    if (_state == MochiState::Idle) {
                        if (now - _lastStateTime > 15000 && random(100) < 5) {
                            setState(MochiState::Happy);
                        }
                    } else if (_state == MochiState::Happy && now - _lastStateTime > 2000) {
                        setState(MochiState::Idle);
                    }
                }
            }
        }
    }
    
    // 6. Handle breathing animation (always running except when sleeping)
    if (_state != MochiState::Sleeping) {
        // Normal breathing speed: 600ms per direction, low battery breathes slower (900ms)
        uint32_t speed = _lowBattery ? 900 : 600;
        if (now - _lastBreathTime >= speed) {
            _lastBreathTime = now;
            _breathUp = !_breathUp;
            _breathOffset = _breathUp ? 1 : 0;
        }
    } else {
        // Sleep breathing is very slow
        if (now - _lastBreathTime >= 1200) {
            _lastBreathTime = now;
            _breathUp = !_breathUp;
            _breathOffset = _breathUp ? 1 : 0;
        }
    }
    
    // 7. Handle looking around and blinking during Idle/Happy/Sleepy/Cold/Hot/Noisy
    if (_state == MochiState::Idle || _state == MochiState::Happy || 
        _state == MochiState::Sleepy || _state == MochiState::Cold || 
        _state == MochiState::Hot || _state == MochiState::Noisy) {
        // Blinking logic
        if (_isBlinking) {
            if (now - _blinkStart >= 150) { // Blink lasts 150ms
                _isBlinking = false;
                _lastBlinkTime = now;
            }
        } else {
            // Blink every 3-6 seconds. If sleepy, blink more often (2-3 seconds)
            uint32_t blinkInterval = _state == MochiState::Sleepy ? random(2000, 3500) : random(4000, 7000);
            if (now - _lastBlinkTime >= blinkInterval) {
                _isBlinking = true;
                _blinkStart = now;
            }
        }
        
        // Look around logic
        if (_state == MochiState::Cold) {
            // Shiver offset updated every tick (fast wobble)
            _lookX = random(3) - 1; // -1, 0, 1
            _lookY = random(3) - 1; // -1, 0, 1
        } else {
            if (!_isBlinking && now - _lastLookTime >= random(3000, 6000)) {
                _lastLookTime = now;
                if (random(100) < 40) { // 40% chance to look away from center
                    _lookX = random(3) - 1; // -1, 0, 1
                    _lookY = random(3) - 1;
                    // Scale look offsets
                    _lookX *= 4;
                    _lookY *= 2;
                } else {
                    _lookX = 0;
                    _lookY = 0;
                }
            }
        }
    } else {
        _isBlinking = false;
        _lookX = 0;
        _lookY = 0;
    }
    
    // 8. Update floating Zzz's positions for Sleeping state
    if (_state == MochiState::Sleeping) {
        if (now - _lastZzzTime >= 100) {
            _lastZzzTime = now;
            _sleepBubbleStep = (_sleepBubbleStep + 1) % 30;
            for (int i = 0; i < 3; i++) {
                // Drift up and right
                _zzzY[i] -= 1;
                if (i == 0) _zzzX[i] += (random(3) - 1); // slight wobble
                else if (i == 1) _zzzX[i] += (random(2)); // drift right
                else _zzzX[i] += (random(3) - 1);
                
                // If it goes off screen, reset to start position near Mochi's head
                if (_zzzY[i] < -8 || _zzzX[i] > 128) {
                    _zzzX[i] = 85 + random(10);
                    _zzzY[i] = 30 + random(5);
                }
            }
        }
    }
}

void MochiFace::draw(IDisplay& d) {
    // Normal coordinates: left eye center = 36, right eye center = 92
    // y center = 28
    int lcx = 36 + _lookX;
    int rcx = 92 + _lookX;
    int cy = 28 + _lookY + _breathOffset;
    
    // Draw eyes
    _drawEye(d, lcx, cy, true);
    _drawEye(d, rcx, cy, false);
    
    // Draw sweat drops if hot
    if (_state == MochiState::Hot) {
        // Sweat drop near left eye
        d.drawLine(lcx - 14, cy - 10, lcx - 14, cy - 8);
        d.drawRect(lcx - 15, cy - 9, 2, 2, true);
        // Sweat drop near right eye
        d.drawLine(rcx + 14, cy - 6, rcx + 14, cy - 4);
        d.drawRect(rcx + 13, cy - 5, 2, 2, true);
    }
    
    // Draw mouth
    int mcx = 64;
    int mcy = 44 + _breathOffset;
    _drawMouth(d, mcx, mcy);
    
    // Draw Zzz's if sleeping
    if (_state == MochiState::Sleeping) {
        _drawZzz(d);
    }
}

void MochiFace::_drawEye(IDisplay& d, int cx, int cy, bool isLeft) {
    if (_isBlinking) {
        // Closed blink: flat horizontal line
        d.drawRect(cx - 10, cy - 1, 20, 3, true);
        return;
    }
    
    switch (_state) {
        case MochiState::Idle:
            // Normal rounded eyes
            d.drawRoundRect(cx - 10, cy - 13, 20, 26, 5, true);
            break;
            
        case MochiState::Happy:
            // Curved happy eyes (arch/inverted U shape)
            d.drawLine(cx - 10, cy + 2, cx - 5, cy - 4);
            d.drawLine(cx - 5, cy - 4, cx + 5, cy - 4);
            d.drawLine(cx + 5, cy - 4, cx + 10, cy + 2);
            // Thickening
            d.drawLine(cx - 10, cy + 3, cx - 5, cy - 3);
            d.drawLine(cx - 5, cy - 3, cx + 5, cy - 3);
            d.drawLine(cx + 5, cy - 3, cx + 10, cy + 3);
            break;
            
        case MochiState::Angry:
            // Normal eye cut off by slanted eyebrow
            d.drawRoundRect(cx - 10, cy - 10, 20, 22, 4, true);
            if (isLeft) {
                // slanted eyebrow slanting down towards center
                d.drawLine(cx - 13, cy - 13, cx + 11, cy - 6);
                d.drawLine(cx - 13, cy - 12, cx + 11, cy - 5);
                d.drawLine(cx - 13, cy - 11, cx + 11, cy - 4);
            } else {
                // slanted eyebrow slanting down towards center
                d.drawLine(cx + 13, cy - 13, cx - 11, cy - 6);
                d.drawLine(cx + 13, cy - 12, cx - 11, cy - 5);
                d.drawLine(cx + 13, cy - 11, cx - 11, cy - 4);
            }
            break;
            
        case MochiState::Dizzy: {
            // Spiral dizzy eyes!
            // Outermost ring:
            d.drawLine(cx - 8, cy - 8, cx + 8, cy - 8); // top
            d.drawLine(cx + 8, cy - 8, cx + 8, cy + 8); // right
            d.drawLine(cx + 8, cy + 8, cx - 8, cy + 8); // bottom
            d.drawLine(cx - 8, cy + 8, cx - 8, cy - 4); // left
            // Inner ring:
            d.drawLine(cx - 8, cy - 4, cx + 4, cy - 4); // inner top
            d.drawLine(cx + 4, cy - 4, cx + 4, cy + 4); // inner right
            d.drawLine(cx + 4, cy + 4, cx - 4, cy + 4); // inner bottom
            d.drawLine(cx - 4, cy + 4, cx - 4, cy);     // inner left
            d.drawLine(cx - 4, cy,     cx,     cy);     // center
            break;
        }
            
        case MochiState::Sleepy:
            // Half closed drooping eyes
            d.drawRoundRect(cx - 10, cy - 3, 20, 16, 4, true);
            // Eyelid line cutting top part
            d.drawLine(cx - 12, cy - 3, cx + 12, cy - 3);
            d.drawLine(cx - 12, cy - 2, cx + 12, cy - 2);
            break;
            
        case MochiState::Sleeping:
            // Curved closed eyes (U-shape)
            d.drawLine(cx - 10, cy - 3, cx - 5, cy + 2);
            d.drawLine(cx - 5, cy + 2, cx + 5, cy + 2);
            d.drawLine(cx + 5, cy + 2, cx + 10, cy - 3);
            // Thickening
            d.drawLine(cx - 10, cy - 2, cx - 5, cy + 3);
            d.drawLine(cx - 5, cy + 3, cx + 5, cy + 3);
            d.drawLine(cx + 5, cy + 3, cx + 10, cy - 2);
            break;
            
        case MochiState::Surprised:
            // Startled round eyes with pupils
            d.drawCircle(cx, cy, 11, false);
            d.drawCircle(cx, cy, 10, false);
            d.drawCircle(cx, cy, 3, true); // pupil
            break;
            
        case MochiState::Cold:
            // Shivering rounded eyes (shivering offset applied in tick)
            d.drawRoundRect(cx - 10, cy - 13, 20, 26, 5, true);
            break;
            
        case MochiState::Hot:
            // Drooping, exhausted eyes
            d.drawRoundRect(cx - 10, cy - 3, 20, 14, 4, true);
            d.drawLine(cx - 12, cy - 3, cx + 12, cy - 3);
            d.drawLine(cx - 12, cy - 2, cx + 12, cy - 2);
            break;
            
        case MochiState::Noisy:
            // Squeezed shut (> <) eyes
            if (isLeft) {
                d.drawLine(cx - 8, cy - 6, cx + 2, cy);
                d.drawLine(cx - 8, cy - 5, cx + 2, cy);
                d.drawLine(cx - 8, cy + 6, cx + 2, cy);
                d.drawLine(cx - 8, cy + 5, cx + 2, cy);
            } else {
                d.drawLine(cx + 8, cy - 6, cx - 2, cy);
                d.drawLine(cx + 8, cy - 5, cx - 2, cy);
                d.drawLine(cx + 8, cy + 6, cx - 2, cy);
                d.drawLine(cx + 8, cy + 5, cx - 2, cy);
            }
            break;
    }
}

void MochiFace::_drawMouth(IDisplay& d, int cx, int cy) {
    switch (_state) {
        case MochiState::Idle:
            // Small line mouth
            d.drawLine(cx - 4, cy, cx + 4, cy);
            break;
            
        case MochiState::Happy:
            // Upward curve mouth (smile)
            d.drawLine(cx - 5, cy - 1, cx, cy + 2);
            d.drawLine(cx, cy + 2, cx + 5, cy - 1);
            d.drawLine(cx - 5, cy, cx, cy + 3);
            d.drawLine(cx, cy + 3, cx + 5, cy);
            break;
            
        case MochiState::Angry:
            // Downward curve mouth (frown)
            d.drawLine(cx - 5, cy + 2, cx, cy - 1);
            d.drawLine(cx, cy - 1, cx + 5, cy + 2);
            d.drawLine(cx - 5, cy + 3, cx, cy);
            d.drawLine(cx, cy, cx + 5, cy + 3);
            break;
            
        case MochiState::Dizzy:
            // Wavy mouth
            d.drawLine(cx - 6, cy, cx - 2, cy - 2);
            d.drawLine(cx - 2, cy - 2, cx + 2, cy + 2);
            d.drawLine(cx + 2, cy + 2, cx + 6, cy);
            break;
            
        case MochiState::Sleepy:
            // Small circle yawning mouth
            d.drawCircle(cx, cy, 3, false);
            break;
            
        case MochiState::Sleeping: {
            // Small dot mouth
            d.drawRect(cx, cy, 2, 2, true);
            
            // Draw inflating/deflating sleeping snot/snore bubble
            int step = _sleepBubbleStep;
            if (step >= 6 && step <= 22) {
                int r = map(step, 6, 22, 1, 5);
                // Draw the bubble offset up and right from the mouth
                d.drawCircle(cx + 4, cy - 3, r, false);
            }
            break;
        }
            
        case MochiState::Surprised:
            // Large circular open mouth
            d.drawCircle(cx, cy, 5, false);
            d.drawCircle(cx, cy, 4, false);
            break;
            
        case MochiState::Cold:
            // Zigzag shivering mouth
            d.drawLine(cx - 5, cy + (random(3) - 1), cx - 2, cy - 2 + (random(3) - 1));
            d.drawLine(cx - 2, cy - 2 + (random(3) - 1), cx + 2, cy + 2 + (random(3) - 1));
            d.drawLine(cx + 2, cy + 2 + (random(3) - 1), cx + 5, cy + (random(3) - 1));
            break;
            
        case MochiState::Hot:
            // Open panting mouth with sticking-out tongue
            d.drawCircle(cx, cy + 1, 4, false);
            d.drawRect(cx - 2, cy + 4, 4, 4, true); // tongue
            break;
            
        case MochiState::Noisy:
            // Grimacing clenched/gritting teeth mouth
            d.drawRoundRect(cx - 8, cy - 3, 16, 7, 2, false);
            d.drawLine(cx - 8, cy, cx + 8, cy); // horizontal division line
            d.drawLine(cx - 4, cy - 3, cx - 4, cy + 3); // vertical teeth divisions
            d.drawLine(cx, cy - 3, cx, cy + 3);
            d.drawLine(cx + 4, cy - 3, cx + 4, cy + 3);
            break;
    }
}

void MochiFace::_drawZzz(IDisplay& d) {
    // Floating Zzz letters with font sizes for natural size progression
    d.drawText(_zzzX[0], _zzzY[0], "z");     // small z (lowercase)
    d.drawText(_zzzX[1], _zzzY[1], "Z");     // medium z (uppercase size 1)
    d.drawText(_zzzX[2], _zzzY[2], "Z", 2);  // large Z (uppercase size 2)
}
