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
    int    _pickIdx   = 0;
    bool   _done      = false;
    bool   _confirmed = false;

    // Full printable ASCII charset (95 chars, index 0-94)
    static constexpr const char CHARSET[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        " !@#$%^&*()-_=+[]{}|;:',.<>?/";
    static constexpr int CHARSET_LEN = 95;

    // The picker scrolls over the charset plus two action tokens at the end,
    // so the whole flow works with only press-down (no long-press needed):
    //   index CHARSET_LEN     -> DEL (backspace)
    //   index CHARSET_LEN + 1 -> OK  (submit; empty = cancel/back)
    static constexpr int IDX_DEL  = CHARSET_LEN;
    static constexpr int IDX_OK   = CHARSET_LEN + 1;
    static constexpr int PICK_LEN = CHARSET_LEN + 2;

    String _pickLabel(int idx) const;  // display label for a picker position
};
