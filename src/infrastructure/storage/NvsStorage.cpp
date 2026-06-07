#include "NvsStorage.h"

static constexpr const char* NS_WIFI = "derabot_wifi";
static constexpr const char* NS_TRIG = "derabot_trig";
static constexpr const char* NS_PWR  = "derabot_pwr";

bool NvsStorage::begin() { return true; }

bool NvsStorage::saveWifi(const WifiCredentials& creds) {
    _prefs.begin(NS_WIFI, false);
    _prefs.putString("ssid", creds.ssid);
    _prefs.putString("pass", creds.password);
    _prefs.end();
    return true;
}

bool NvsStorage::loadWifi(WifiCredentials& creds) {
    _prefs.begin(NS_WIFI, true);
    creds.ssid     = _prefs.getString("ssid", "");
    creds.password = _prefs.getString("pass", "");
    _prefs.end();
    return !creds.ssid.isEmpty();
}

bool NvsStorage::saveThreshold(const ThresholdConfig& cfg) {
    _prefs.begin(NS_TRIG, false);
    _prefs.putInt("ver",   1);  // mark as migrated
    _prefs.putFloat("tmax",  cfg.tempMax);
    _prefs.putFloat("tmin",  cfg.tempMin);
    _prefs.putFloat("hmax",  cfg.humidMax);
    _prefs.putFloat("pmax",  cfg.pressMax);
    _prefs.putFloat("smax",  cfg.soundMax);
    _prefs.putFloat("vmin",  cfg.voltMin);
    _prefs.putFloat("bmin",  cfg.battMin);
    _prefs.putFloat("bmax",  cfg.battMax);
    _prefs.putBool("en",     cfg.enabled);
    _prefs.end();
    return true;
}

bool NvsStorage::loadThreshold(ThresholdConfig& cfg) {
    _prefs.begin(NS_TRIG, false);  // read-write so migration can write
    // ver==0 means old firmware or fresh flash — force alarm disabled
    if (_prefs.getInt("ver", 0) == 0) {
        _prefs.clear();
        _prefs.putInt("ver",   1);
        _prefs.putFloat("tmax",  40.0f);
        _prefs.putFloat("tmin",  0.0f);
        _prefs.putFloat("hmax",  85.0f);
        _prefs.putFloat("pmax",  1050.0f);
        _prefs.putFloat("smax",  80.0f);
        _prefs.putFloat("vmin",  3.3f);
        _prefs.putBool("en",     false);
    }
    cfg.tempMax  = _prefs.getFloat("tmax",  40.0f);
    cfg.tempMin  = _prefs.getFloat("tmin",  0.0f);
    cfg.humidMax = _prefs.getFloat("hmax",  85.0f);
    cfg.pressMax = _prefs.getFloat("pmax",  1050.0f);
    cfg.soundMax = _prefs.getFloat("smax",  80.0f);
    cfg.voltMin  = _prefs.getFloat("vmin",  3.3f);
    cfg.battMin  = _prefs.getFloat("bmin",  3.2f);  // default = old hardcoded gauge
    cfg.battMax  = _prefs.getFloat("bmax",  3.7f);
    cfg.enabled  = _prefs.getBool("en",     false);
    _prefs.end();
    return true;
}

bool NvsStorage::saveEco(bool on) {
    _prefs.begin(NS_PWR, false);
    _prefs.putBool("eco", on);
    _prefs.end();
    return true;
}

bool NvsStorage::loadEco(bool& on) {
    _prefs.begin(NS_PWR, true);
    on = _prefs.getBool("eco", false);  // default OFF on fresh flash
    _prefs.end();
    return true;
}

void NvsStorage::clear() {
    _prefs.begin(NS_WIFI, false); _prefs.clear(); _prefs.end();
    _prefs.begin(NS_TRIG, false); _prefs.clear(); _prefs.end();
    _prefs.begin(NS_PWR,  false); _prefs.clear(); _prefs.end();
}
