#include "YahooStockClient.h"
#include "../../config/config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool YahooStockClient::fetch(StockData& out) {
    out.valid = false;
    out.error = "";

    if (WiFi.status() != WL_CONNECTED) {
        out.error = "offline";
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();            // skip cert validation (no CA store on device)
    client.setTimeout(8000);

    HTTPClient https;
    String url = String("https://") + Config::Stock::HOST + Config::Stock::PATH;
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

    // Keep only the fields we actually use — streaming filter keeps RAM tiny.
    JsonDocument filter;
    filter["chart"]["result"][0]["meta"]["regularMarketPrice"] = true;
    filter["chart"]["result"][0]["meta"]["chartPreviousClose"] = true;
    filter["chart"]["result"][0]["meta"]["previousClose"]      = true;
    filter["chart"]["result"][0]["indicators"]["quote"][0]["close"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(
        doc, https.getStream(), DeserializationOption::Filter(filter));
    https.end();
    if (err) {
        out.error = String("json:") + err.c_str();
        return false;
    }

    JsonObject result = doc["chart"]["result"][0];
    if (result.isNull()) { out.error = "no result"; return false; }

    JsonObject meta = result["meta"];
    out.price     = meta["regularMarketPrice"] | 0.0f;
    out.prevClose = meta["chartPreviousClose"] | (meta["previousClose"] | 0.0f);

    // Collect the non-null intraday closes (Yahoo emits null for gaps).
    std::vector<float> tmp;
    JsonArray closes = result["indicators"]["quote"][0]["close"];
    if (!closes.isNull()) {
        tmp.reserve(closes.size());
        for (JsonVariant v : closes)
            if (!v.isNull()) tmp.push_back(v.as<float>());
    }

    // Downsample to fit the screen width.
    out.points.clear();
    const int maxp = Config::Stock::MAX_POINTS;
    const int n    = tmp.size();
    if (n <= maxp) {
        out.points = tmp;
    } else {
        out.points.reserve(maxp);
        for (int i = 0; i < maxp; i++)
            out.points.push_back(tmp[(long)i * (n - 1) / (maxp - 1)]);
    }

    if (out.price <= 0 && !tmp.empty())          out.price     = tmp.back();
    if (out.prevClose <= 0 && !out.points.empty()) out.prevClose = out.points.front();

    out.change    = out.price - out.prevClose;
    out.changePct = (out.prevClose > 0) ? (out.change / out.prevClose * 100.0f) : 0.0f;
    out.valid     = (out.price > 0);
    out.fetchedAt = millis();
    if (!out.valid) out.error = "no data";
    return out.valid;
}
