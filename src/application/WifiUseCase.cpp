#include "WifiUseCase.h"
#include "../config/config.h"

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
void    WifiUseCase::setPowerSave(bool on) { _wifi->setPowerSave(on); }

bool   WifiUseCase::startAP()        { return _wifi->startAP(Config::Ap::SSID, Config::Ap::PASS); }
void   WifiUseCase::stopAP()         { _wifi->stopAP(); }
String WifiUseCase::apIp()     const { return _wifi->getApIp(); }
int    WifiUseCase::apClients()const { return _wifi->apClients(); }
