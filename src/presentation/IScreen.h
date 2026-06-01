#pragma once
#include "../domain/interfaces/IDisplay.h"

enum class ButtonEvent { None, Left, Center, Right, LongCenter };

class IScreen {
public:
    virtual ~IScreen() = default;
    virtual void enter() {}
    virtual void exit()  {}
    virtual void update(IDisplay& display) = 0;
    virtual void onButton(ButtonEvent evt) = 0;
    virtual bool isDone() const { return false; }
};
