#include "WifiUseCase.h"

WifiUseCase::WifiUseCase(IWifiManager* wifi, IStorage* storage)
    : _wifi(wifi), _storage(storage) {}

bool WifiUseCase::begin() {
    return _wifi->begin();
}

bool WifiUseCase::autoConnect() {
    if (!_storage->loadWifi(_saved)) return false;
    if (_saved.ssid.isEmpty()) return false;
    return _wifi->connect(_saved);
}

bool WifiUseCase::connect(const String& ssid, const String& password) {
    WifiCredentials creds{ ssid, password };
    if (_wifi->connect(creds)) {
        _storage->saveWifi(creds);
        _saved = creds;
        return true;
    }
    return false;
}

void WifiUseCase::disconnect() { _wifi->disconnect(); }
bool WifiUseCase::isConnected() const { return _wifi->isConnected(); }
std::vector<WifiNetwork> WifiUseCase::scan() { return _wifi->scan(); }
int32_t WifiUseCase::getRssi() const { return _wifi->getRssi(); }
String  WifiUseCase::getIp()   const { return _wifi->getIp(); }
