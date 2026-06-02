#include "WifiPortal.h"
#include <WiFi.h>

void WifiPortal::begin(ScanFn scan) {
    _scanFn    = scan;
    _submitted = false;
    _nets      = _scanFn ? _scanFn() : std::vector<WifiNetwork>{};

    _server.on("/",     HTTP_GET,  [this]() { _handleRoot(); });
    _server.on("/save", HTTP_POST, [this]() { _handleSave(); });
    _server.onNotFound([this]() { _handleNotFound(); });

    // Captive portal: resolve every hostname to our AP IP so phones pop the
    // "sign in to network" page automatically.
    _dns.start(53, "*", WiFi.softAPIP());
    _server.begin();
    _running = true;
}

void WifiPortal::loop() {
    if (!_running) return;
    _dns.processNextRequest();
    _server.handleClient();
}

void WifiPortal::stop() {
    if (!_running) return;
    _server.stop();
    _dns.stop();
    _running = false;
}

void WifiPortal::_handleRoot() {
    if (_server.hasArg("rescan") && _scanFn) _nets = _scanFn();
    _server.send(200, "text/html", _pageHtml());
}

void WifiPortal::_handleSave() {
    String picked = _server.arg("ssid");
    String manual = _server.arg("ssid_manual");
    _ssid     = manual.length() ? manual : picked;
    _password = _server.arg("pass");
    _submitted = !_ssid.isEmpty();

    String h = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
                 "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                 "<style>body{font-family:sans-serif;background:#111;color:#eee;"
                 "text-align:center;padding:2em}b{color:#2d7}</style></head><body>");
    if (_submitted)
        h += "<h2>Tersimpan &#10003;</h2><p>Menghubungkan ke <b>" + _ssid +
             "</b>...</p><p>Anda bisa menutup halaman ini. Lihat layar perangkat untuk status.</p>";
    else
        h += F("<h2>Gagal</h2><p>SSID kosong. <a href='/'>Kembali</a></p>");
    h += F("</body></html>");
    _server.send(200, "text/html", h);
}

void WifiPortal::_handleNotFound() {
    // Redirect any other URL to the portal root (triggers captive-portal popup)
    _server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    _server.send(302, "text/plain", "");
}

String WifiPortal::_pageHtml() const {
    String h = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
                 "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                 "<title>DeraBot WiFi</title><style>"
                 "body{font-family:sans-serif;margin:0;background:#111;color:#eee;padding:1em}"
                 ".card{max-width:420px;margin:auto;background:#1c1c1c;padding:1.2em;border-radius:12px}"
                 "h2{margin-top:0}label{display:block;margin:.6em 0 .2em;font-size:.9em}"
                 "select,input{width:100%;padding:.6em;border-radius:8px;border:1px solid #444;"
                 "background:#222;color:#eee;box-sizing:border-box;font-size:1em}"
                 "button{width:100%;padding:.8em;margin-top:1em;border:0;border-radius:8px;"
                 "background:#2d7;color:#000;font-weight:bold;font-size:1em}"
                 "a{color:#2d7}.c{text-align:center}</style></head><body><div class='card'>"
                 "<h2>Setup WiFi DeraBot</h2>"
                 "<form method='POST' action='/save'>"
                 "<label>Pilih jaringan</label><select name='ssid'>");
    for (const auto& n : _nets) {
        h += "<option value='" + n.ssid + "'>" + n.ssid +
             (n.secured ? " &#128274;" : "") + " (" + String(n.rssi) + "dBm)</option>";
    }
    h += F("</select>"
           "<label>atau ketik SSID manual (opsional)</label>"
           "<input name='ssid_manual' placeholder='nama WiFi'>"
           "<label>Password</label>"
           "<input name='pass' type='password' placeholder='password WiFi'>"
           "<button type='submit'>Simpan &amp; Hubungkan</button></form>"
           "<p class='c'><a href='/?rescan=1'>&#8635; Scan ulang</a></p>"
           "</div></body></html>");
    return h;
}
