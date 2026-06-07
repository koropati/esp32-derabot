#include "ErApiClient.h"
#include "../../config/config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool ErApiClient::fetch(ExchangeData& out) {
    out.valid = false;
    out.error = "";
    out.rates.clear();

    if (WiFi.status() != WL_CONNECTED) {
        out.error = "offline";
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();            // skip cert validation (no CA store on device)
    client.setTimeout(8000);

    HTTPClient https;
    String url = String("https://") + Config::Forex::HOST + Config::Forex::PATH;
    if (!https.begin(client, url)) {
        out.error = "begin fail";
        return false;
    }
    https.setUserAgent("Mozilla/5.0");
    https.setConnectTimeout(8000);
    https.setTimeout(8000);

    int code = https.GET();
    if (code != HTTP_CODE_OK) {
        out.error = "HTTP " + String(code);
        https.end();
        return false;
    }

    // Keep only the fields we use: the result flag, update text, IDR, and each
    // configured currency's USD rate — streaming filter keeps RAM tiny.
    JsonDocument filter;
    filter["result"]                     = true;
    filter["time_last_update_utc"]       = true;
    filter["rates"]["IDR"]               = true;
    for (int i = 0; i < Config::Forex::CODES_COUNT; i++)
        filter["rates"][Config::Forex::CODES[i]] = true;

    // Read the full body via getString() (NOT getStream()): this endpoint replies
    // with Transfer-Encoding: chunked, and only getString() de-chunks it. Feeding
    // the raw chunked stream to the parser yields a broken document.
    String body = https.getString();
    https.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(
        doc, body, DeserializationOption::Filter(filter));
    if (err) {
        out.error = String("json:") + err.c_str();
        return false;
    }

    if (String(doc["result"] | "") != "success") {
        out.error = "api error";
        return false;
    }

    float idrPerUsd = doc["rates"]["IDR"] | 0.0f;
    if (idrPerUsd <= 0) { out.error = "no IDR"; return false; }

    // Short, friendly update text: "Sat, 07 Jun 2025 00:02 ..." -> "07 Jun 2025"
    String upd = doc["time_last_update_utc"] | "";
    out.updated = (upd.length() >= 16) ? upd.substring(5, 16) : upd;

    // 1 unit of CODE in IDR = (IDR per USD) / (CODE per USD).
    for (int i = 0; i < Config::Forex::CODES_COUNT; i++) {
        const char* c = Config::Forex::CODES[i];
        float perUsd = doc["rates"][c] | 0.0f;
        if (perUsd <= 0) continue;            // currency missing from feed — skip
        ExchangeRate r;
        r.code = String(c);
        r.idr  = idrPerUsd / perUsd;
        out.rates.push_back(r);
    }

    out.valid     = !out.rates.empty();
    out.fetchedAt = millis();
    if (!out.valid) out.error = "no data";
    return out.valid;
}
