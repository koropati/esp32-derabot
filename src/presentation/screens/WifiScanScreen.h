#pragma once
#include "../IScreen.h"
#include "../../application/WifiUseCase.h"
#include <vector>

class WifiScanScreen : public IScreen {
public:
    explicit WifiScanScreen(WifiUseCase* wifi);
    void   enter() override;
    void   update(IDisplay& display) override;
    void   onButton(ButtonEvent evt) override;
    bool   isDone() const override { return _done; }
    String selectedSsid() const    { return _selectedSsid; }
    bool   wantConnect()  const    { return _wantConnect; }

private:
    WifiUseCase*             _wifi;
    std::vector<WifiNetwork> _networks;
    int                      _cursor   = 0;
    bool                     _done     = false;
    bool                     _scanning = false;
    bool                     _wantConnect = false;
    String                   _selectedSsid;
};
