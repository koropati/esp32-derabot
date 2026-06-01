#pragma once
#include "../IScreen.h"

class WifiPasswordScreen : public IScreen {
public:
    explicit WifiPasswordScreen(const String& ssid);
    void   enter() override;
    void   update(IDisplay& display) override;
    void   onButton(ButtonEvent evt) override;
    bool   isDone()    const override { return _done; }
    String password()  const { return _password; }
    bool   confirmed() const { return _confirmed; }

private:
    String _ssid;
    String _password;
    int    _charIdx   = 0;
    bool   _done      = false;
    bool   _confirmed = false;

    // Full printable ASCII charset (95 chars, index 0-94)
    static constexpr const char CHARSET[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        " !@#$%^&*()-_=+[]{}|;:',.<>?/";
    static constexpr int CHARSET_LEN = 95;
};
