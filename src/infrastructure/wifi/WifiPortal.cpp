#include "WifiPortal.h"
#include <WiFi.h>
#include <algorithm>

void WifiPortal::begin(std::vector<WifiNetwork> nets, ScanFn rescan) {
    _scanFn    = rescan;
    _submitted = false;
    _nets      = _sanitize(std::move(nets));

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
    if (_server.hasArg("rescan") && _scanFn) _nets = _sanitize(_scanFn());
    _server.send(200, "text/html", _pageHtml());
}

// Drop empty/hidden SSIDs, collapse duplicates (keep the strongest signal of
// each name), then sort strongest-first so the closest networks are on top.
std::vector<WifiNetwork> WifiPortal::_sanitize(std::vector<WifiNetwork> nets) {
    std::sort(nets.begin(), nets.end(),
              [](const WifiNetwork& a, const WifiNetwork& b) { return a.rssi > b.rssi; });
    std::vector<WifiNetwork> out;
    for (auto& n : nets) {
        if (n.ssid.isEmpty()) continue;
        bool dup = false;
        for (auto& o : out) if (o.ssid == n.ssid) { dup = true; break; }
        if (!dup) out.push_back(n);
    }
    return out;
}

void WifiPortal::_handleSave() {
    String picked = _server.arg("ssid");
    String manual = _server.arg("ssid_manual");
    _ssid     = manual.length() ? manual : picked;
    _password = _server.arg("pass");
    _submitted = !_ssid.isEmpty();

    String h = F("<!DOCTYPE html><html lang='id'><head><meta charset='utf-8'>"
                 "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                 "<title>DeraBot WiFi</title><style>"
                 "*{box-sizing:border-box}body{font-family:-apple-system,Segoe UI,Roboto,sans-serif;"
                 "margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;"
                 "background:linear-gradient(160deg,#0b1220,#10243a);color:#e9eef5;padding:16px}"
                 ".card{width:100%;max-width:400px;background:#141c2b;border:1px solid #233246;"
                 "border-radius:18px;padding:26px 22px;text-align:center;"
                 "box-shadow:0 12px 40px rgba(0,0,0,.45)}"
                 "h2{margin:.2em 0}b{color:#3ad29f}p{color:#9fb3cc;line-height:1.5}"
                 ".ico{font-size:2.6em;line-height:1}"
                 "a{display:inline-block;margin-top:14px;color:#3ad29f;text-decoration:none;"
                 "font-weight:600}</style></head><body><div class='card'>");
    if (_submitted)
        h += "<div class='ico'>&#10003;</div><h2>Tersimpan</h2>"
             "<p>Menghubungkan ke <b>" + _ssid + "</b>&hellip;</p>"
             "<p>Anda bisa menutup halaman ini.<br>Lihat layar perangkat untuk status.</p>";
    else
        h += F("<div class='ico'>&#9888;</div><h2>Gagal</h2>"
               "<p>Nama WiFi (SSID) masih kosong.</p><a href='/'>&#8592; Kembali</a>");
    h += F("</div></body></html>");
    _server.send(200, "text/html", h);
}

void WifiPortal::_handleNotFound() {
    // Redirect any other URL to the portal root (triggers captive-portal popup)
    _server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    _server.send(302, "text/plain", "");
}

String WifiPortal::_pageHtml() const {
    String h = F("<!DOCTYPE html><html lang='id'><head><meta charset='utf-8'>"
                 "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                 "<title>DeraBot WiFi</title><style>"
                 "*{box-sizing:border-box}"
                 "body{font-family:-apple-system,Segoe UI,Roboto,sans-serif;margin:0;"
                 "min-height:100vh;display:flex;align-items:center;justify-content:center;"
                 "background:linear-gradient(160deg,#0b1220,#10243a);color:#e9eef5;padding:16px}"
                 ".card{width:100%;max-width:400px;background:#141c2b;border:1px solid #233246;"
                 "border-radius:18px;padding:24px 22px;box-shadow:0 12px 40px rgba(0,0,0,.45)}"
                 ".logo{text-align:center;font-size:1.5em;font-weight:700;margin:0}"
                 ".sub{text-align:center;color:#8aa0bb;font-size:.85em;margin:2px 0 8px}"
                 "label{display:block;margin:14px 0 6px;font-size:.8em;color:#9fb3cc;font-weight:600}"
                 "select,input{width:100%;padding:12px;border-radius:10px;border:1px solid #2c3e57;"
                 "background:#0e1726;color:#e9eef5;font-size:1em;outline:none}"
                 "select:focus,input:focus{border-color:#3ad29f}"
                 ".pw{position:relative}.pw input{padding-right:74px}"
                 ".eye{position:absolute;right:6px;top:6px;bottom:6px;width:62px;padding:0;"
                 "background:#22324a;border:0;border-radius:8px;color:#cfe0f3;font-size:.82em;"
                 "font-weight:600;cursor:pointer}"
                 "button.submit{width:100%;padding:14px;margin-top:22px;border:0;border-radius:10px;"
                 "background:linear-gradient(135deg,#3ad29f,#2bb6c4);color:#04121a;font-weight:700;"
                 "font-size:1.02em;cursor:pointer}"
                 ".rescan{display:block;text-align:center;margin-top:16px;color:#3ad29f;"
                 "text-decoration:none;font-size:.9em}"
                 ".empty{margin-top:6px;background:#1a2435;border:1px dashed #2c3e57;border-radius:10px;"
                 "padding:12px;color:#9fb3cc;font-size:.85em;text-align:center}"
                 "</style></head><body><div class='card'>"
                 "<div class='logo'>DeraBot</div>"
                 "<div class='sub'>Pengaturan Koneksi WiFi</div>"
                 "<form method='POST' action='/save'>"
                 "<label>Pilih Jaringan WiFi</label>");
    if (_nets.empty()) {
        h += F("<div class='empty'>Tidak ada jaringan ditemukan.<br>"
               "Coba pindai ulang atau ketik nama WiFi manual di bawah.</div>");
    } else {
        h += F("<select name='ssid'>");
        for (const auto& n : _nets) {
            h += "<option value='" + n.ssid + "'>" + n.ssid +
                 (n.secured ? " &#128274;" : "") + "  " + _bars(n.rssi) + "</option>";
        }
        h += F("</select>");
    }
    h += F("<label>atau Ketik Nama WiFi (opsional)</label>"
           "<input name='ssid_manual' placeholder='nama WiFi manual'>"
           "<label>Password</label>"
           "<div class='pw'>"
           "<input id='pw' name='pass' type='password' placeholder='password WiFi' autocomplete='off'>"
           "<button type='button' id='eye' class='eye'>Lihat</button>"
           "</div>"
           "<button type='submit' class='submit'>Simpan &amp; Hubungkan</button></form>"
           "<a class='rescan' href='/?rescan=1'>&#8635; Pindai ulang jaringan</a>"
           "<script>(function(){var p=document.getElementById('pw'),b=document.getElementById('eye');"
           "b.onclick=function(){var t=p.type==='password';p.type=t?'text':'password';"
           "b.textContent=t?'Tutup':'Lihat';};})();</script>"
           "</div></body></html>");
    return h;
}

// Compact signal indicator from RSSI (dBm) for the dropdown labels.
String WifiPortal::_bars(int rssi) {
    if (rssi >= -55) return F("\xe2\x96\x82\xe2\x96\x84\xe2\x96\x86\xe2\x96\x88");  // ▂▄▆█
    if (rssi >= -65) return F("\xe2\x96\x82\xe2\x96\x84\xe2\x96\x86 ");            // ▂▄▆
    if (rssi >= -75) return F("\xe2\x96\x82\xe2\x96\x84  ");                       // ▂▄
    return F("\xe2\x96\x82   ");                                                    // ▂
}
