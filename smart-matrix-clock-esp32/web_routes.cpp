/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include "web_routes.h"
#include "config.h"
#include "globals.h"
#include "persistence.h"
#include "locale_data.h"
#include "text_encoding.h"
#include "wifi_manager.h"
#include "ntp.h"
#include "web_page.h"
#include "display.h"
#include "data_fetcher.h"

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Send a simple {"ok":true} or {"ok":false,"error":"..."} JSON response.
static void _sendOk(AsyncWebServerRequest* req) {
    req->send(200, "application/json", "{\"ok\":true}");
}

static void _sendError(AsyncWebServerRequest* req, int code, const char* msg) {
    JsonDocument doc;
    doc["ok"]    = false;
    doc["error"] = msg;
    String body;
    serializeJson(doc, body);
    req->send(code, "application/json", body);
}

// ─── GET / ────────────────────────────────────────────────────────────────────

static void _handleRoot(AsyncWebServerRequest* req) {
    req->send(200, "text/html", WEB_PAGE_HTML);
}

// ─── GET /api/status ─────────────────────────────────────────────────────────

static void _handleGetStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;

    doc["ntp_synced"]  = ntpSynced;
    doc["active_slot"] = activeSlot;
    doc["ssid"]        = WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "";
    doc["ip"]          = WiFi.status() == WL_CONNECTED
                             ? WiFi.localIP().toString().c_str()
                             : WiFi.softAPIP().toString().c_str();

    // Current time string for the live preview
    if (ntpSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo) && (timeinfo.tm_year + 1900) >= 2020) {
            char tbuf[6];
            snprintf(tbuf, sizeof(tbuf), "%02d:%02d",
                     timeinfo.tm_hour, timeinfo.tm_min);
            doc["time_str"] = tbuf;
        } else {
            doc["time_str"] = "--:--";
        }
    } else {
        doc["time_str"] = "--:--";
    }

    // Weather cache snapshot for the web panel
    doc["weather_cache_valid"] = weatherCache.valid;
    doc["weather_cache_stale"] = weatherCache.stale;
    if (weatherCache.valid) {
        bool isFahrenheit = (cfgTempUnit[0] == 'F' || cfgTempUnit[0] == 'f');
        char preview[64];
        int  temp = (int)(weatherCache.temp + (weatherCache.temp >= 0 ? 0.5f : -0.5f));
        int  tmin = (int)(weatherCache.minTemp + (weatherCache.minTemp >= 0 ? 0.5f : -0.5f));
        int  tmax = (int)(weatherCache.maxTemp + (weatherCache.maxTemp >= 0 ? 0.5f : -0.5f));
        snprintf(preview, sizeof(preview), "%s%d°%c %s  Min%d  Max%d",
                 weatherCache.stale ? "*" : "",
                 temp, isFahrenheit ? 'F' : 'C',
                 weatherCache.condition, tmin, tmax);
        doc["weather_preview"] = preview;
    }

    // Quotes cache snapshot for the web panel
    doc["quotes_cache_valid"] = quoteCacheCount > 0;
    doc["quotes_cache_stale"] = quotesCacheStale;
    if (quoteCacheCount > 0) {
        char preview[SCROLL_BUF_LEN];
        preview[0] = '\0';
        if (quotesCacheStale) strncat(preview, "*", sizeof(preview) - strlen(preview) - 1);
        for (uint8_t i = 0; i < quoteCacheCount; i++) {
            char entry[48];
            snprintf(entry, sizeof(entry), "%s%s: %.2f %c%.2f%%",
                     (i > 0) ? "  " : "",
                     quoteCache[i].symbol,
                     quoteCache[i].price,
                     (quoteCache[i].changePercent >= 0) ? '+' : '-',
                     fabsf(quoteCache[i].changePercent));
            strncat(preview, entry, sizeof(preview) - strlen(preview) - 1);
        }
        doc["quotes_preview"] = preview;
    }

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ─── GET /api/config ─────────────────────────────────────────────────────────

static void _handleGetConfig(AsyncWebServerRequest* req) {
    JsonDocument doc;

    doc["brightness"]       = currentBrightness;
    doc["scroll_speed_ms"]  = scrollSpeed;
    doc["timezone"]         = cfgTimezone;
    doc["language"]         = cfgLanguage;
    doc["ntp_server"]       = cfgNtpServer;
    doc["date_interval_ms"] = cfgDateIntervalMs;
    doc["date_enabled"]     = cfgDateEnabled;
    doc["ui_language"]      = cfgUiLanguage;

    doc["weather_enabled"]    = slotEnabled[2];
    doc["weather_update_ms"]  = cfgWeatherUpdateMs;
    doc["weather_display_ms"] = slotIntervalMs[2];
    doc["weather_lat"]        = cfgWeatherLat;
    doc["weather_lon"]        = cfgWeatherLon;
    doc["temp_unit"]          = cfgTempUnit;

    doc["quotes_enabled"]    = slotEnabled[3];
    doc["quotes_update_ms"]  = cfgQuotesUpdateMs;
    doc["quotes_display_ms"] = slotIntervalMs[3];
    doc["quotes_tickers"]    = cfgQuotesTickers;

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ─── POST /api/config ────────────────────────────────────────────────────────

static void _handlePostConfig(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                              size_t index, size_t total) {
    // Accumulate body chunks (ESPAsyncWebServer calls this handler per chunk)
    (void)index; (void)total;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        _sendError(req, 400, "Invalid JSON");
        return;
    }

    bool changed = false;

    // ── brightness ────────────────────────────────────────────────────────────
    if (doc["brightness"].is<int>()) {
        int v = doc["brightness"].as<int>();
        if (v < 0 || v > 15) { _sendError(req, 400, "brightness must be 0-15"); return; }
        setBrightness((uint8_t)v);
        changed = true;
    }

    // ── scroll_speed_ms ───────────────────────────────────────────────────────
    if (doc["scroll_speed_ms"].is<int>()) {
        int v = doc["scroll_speed_ms"].as<int>();
        if (v < 10 || v > 200) { _sendError(req, 400, "scroll_speed_ms must be 10-200"); return; }
        setScrollSpeed((uint16_t)v);
        changed = true;
    }

    // ── timezone ──────────────────────────────────────────────────────────────
    if (doc["timezone"].is<const char*>()) {
        const char* tz = doc["timezone"].as<const char*>();
        if (ianaToPostfix(tz) == nullptr) { _sendError(req, 400, "Unknown timezone"); return; }
        strncpy(cfgTimezone, tz, NTP_TIMEZONE_MAX - 1);
        cfgTimezone[NTP_TIMEZONE_MAX - 1] = '\0';
        applyTimezone();
        changed = true;
    }

    // ── language ──────────────────────────────────────────────────────────────
    if (doc["language"].is<const char*>()) {
        const char* lang = doc["language"].as<const char*>();
        if (strcmp(lang, "pt") != 0 && strcmp(lang, "en") != 0) {
            _sendError(req, 400, "language must be 'pt' or 'en'"); return;
        }
        strncpy(cfgLanguage, lang, LANG_CODE_MAX - 1);
        cfgLanguage[LANG_CODE_MAX - 1] = '\0';
        changed = true;
    }

    // ── ntp_server ────────────────────────────────────────────────────────────
    if (doc["ntp_server"].is<const char*>()) {
        const char* srv = doc["ntp_server"].as<const char*>();
        if (strlen(srv) == 0 || strlen(srv) >= NTP_SERVER_MAX) {
            _sendError(req, 400, "ntp_server invalid length"); return;
        }
        strncpy(cfgNtpServer, srv, NTP_SERVER_MAX - 1);
        cfgNtpServer[NTP_SERVER_MAX - 1] = '\0';
        changed = true;
    }

    // ── date_interval_ms ──────────────────────────────────────────────────────
    if (doc["date_interval_ms"].is<long>()) {
        long v = doc["date_interval_ms"].as<long>();
        if (v < (long)DATE_INTERVAL_MIN_MS || v > (long)DATE_INTERVAL_MAX_MS) {
            _sendError(req, 400, "date_interval_ms out of range"); return;
        }
        cfgDateIntervalMs = (uint32_t)v;
        changed = true;
    }

    // ── date_enabled ──────────────────────────────────────────────────────────
    if (doc["date_enabled"].is<bool>()) {
        cfgDateEnabled = doc["date_enabled"].as<bool>();
        changed = true;
    }

    // ── ui_language ───────────────────────────────────────────────────────────
    if (doc["ui_language"].is<const char*>()) {
        const char* lang = doc["ui_language"].as<const char*>();
        if (!isUiLanguageValid(lang)) {
            _sendError(req, 400, "Unknown ui_language"); return;
        }
        strncpy(cfgUiLanguage, lang, UI_LANG_CODE_MAX - 1);
        cfgUiLanguage[UI_LANG_CODE_MAX - 1] = '\0';
        changed = true;
    }

    // ── weather_enabled ───────────────────────────────────────────────────────
    if (doc["weather_enabled"].is<bool>()) {
        slotEnabled[2] = doc["weather_enabled"].as<bool>();
        changed = true;
    }
    if (doc["weather_display_ms"].is<long>()) {
        long v = doc["weather_display_ms"].as<long>();
        if (v < (long)WEATHER_DISPLAY_MIN_MS || v > (long)WEATHER_DISPLAY_MAX_MS) {
            _sendError(req, 400, "weather_display_ms out of range"); return;
        }
        slotIntervalMs[2] = (uint32_t)v;
        changed = true;
    }
    if (doc["weather_update_ms"].is<long>()) {
        long v = doc["weather_update_ms"].as<long>();
        if (v < (long)WEATHER_UPDATE_MIN_MS || v > (long)WEATHER_UPDATE_MAX_MS) {
            _sendError(req, 400, "weather_update_ms out of range"); return;
        }
        cfgWeatherUpdateMs = (uint32_t)v;
        changed = true;
    }
    if (doc["weather_lat"].is<float>()) {
        float v = doc["weather_lat"].as<float>();
        if (v < -90.0f || v > 90.0f) { _sendError(req, 400, "weather_lat out of range"); return; }
        cfgWeatherLat = v;
        fetcherReset();
        changed = true;
    }
    if (doc["weather_lon"].is<float>()) {
        float v = doc["weather_lon"].as<float>();
        if (v < -180.0f || v > 180.0f) { _sendError(req, 400, "weather_lon out of range"); return; }
        cfgWeatherLon = v;
        fetcherReset();
        changed = true;
    }
    if (doc["temp_unit"].is<const char*>()) {
        const char* u = doc["temp_unit"].as<const char*>();
        if (strcmp(u, "C") != 0 && strcmp(u, "F") != 0) {
            _sendError(req, 400, "temp_unit must be 'C' or 'F'"); return;
        }
        strncpy(cfgTempUnit, u, WEATHER_TEMP_UNIT_MAX - 1);
        cfgTempUnit[WEATHER_TEMP_UNIT_MAX - 1] = '\0';
        fetcherReset();
        changed = true;
    }

    // ── quotes_enabled ────────────────────────────────────────────────────────
    if (doc["quotes_enabled"].is<bool>()) {
        slotEnabled[3] = doc["quotes_enabled"].as<bool>();
        changed = true;
    }
    if (doc["quotes_display_ms"].is<long>()) {
        long v = doc["quotes_display_ms"].as<long>();
        if (v < (long)QUOTES_DISPLAY_MIN_MS || v > (long)QUOTES_DISPLAY_MAX_MS) {
            _sendError(req, 400, "quotes_display_ms out of range"); return;
        }
        slotIntervalMs[3] = (uint32_t)v;
        changed = true;
    }
    if (doc["quotes_update_ms"].is<long>()) {
        long v = doc["quotes_update_ms"].as<long>();
        if (v < (long)QUOTES_UPDATE_MIN_MS || v > (long)QUOTES_UPDATE_MAX_MS) {
            _sendError(req, 400, "quotes_update_ms out of range"); return;
        }
        cfgQuotesUpdateMs = (uint32_t)v;
        changed = true;
    }
    if (doc["quotes_tickers"].is<const char*>()) {
        const char* v = doc["quotes_tickers"].as<const char*>();
        if (strlen(v) >= QUOTES_TICKERS_MAX) {
            _sendError(req, 400, "quotes_tickers too long"); return;
        }
        strncpy(cfgQuotesTickers, v, QUOTES_TICKERS_MAX - 1);
        cfgQuotesTickers[QUOTES_TICKERS_MAX - 1] = '\0';
        quoteCacheCount  = 0;   // stale symbol list — drop cache until next fetch
        quotesCacheStale = false;
        quotesFetcherReset();
        changed = true;
    }

    if (changed) saveConfig();
    _sendOk(req);
}

// ─── POST /api/alert ─────────────────────────────────────────────────────────

static void _handlePostAlert(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                             size_t index, size_t total) {
    (void)index; (void)total;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) { _sendError(req, 400, "Invalid JSON"); return; }

    if (!doc["message"].is<const char*>()) {
        _sendError(req, 400, "Missing 'message' field"); return;
    }

    const char* utf8msg = doc["message"].as<const char*>();
    if (strlen(utf8msg) == 0) { _sendError(req, 400, "message is empty"); return; }

    // Convert UTF-8 → Latin-1, then resolve [icon] tags to CP437 special glyphs
    // (display driver expects Latin-1, with control bytes as icon glyphs)
    char latin1msg[MAX_ALERT_LEN];
    utf8ToLatin1(utf8msg, latin1msg, MAX_ALERT_LEN);
    expandIconTags(latin1msg, alertMessage, MAX_ALERT_LEN);

    // Optional: mode and duration (only relevant for non-scroll modes)
    if (doc["mode"].is<int>()) {
        int m = doc["mode"].as<int>();
        if (m < 0 || m > 3) { _sendError(req, 400, "mode must be 0-3"); return; }
        alertMode = (uint8_t)m;
    }
    if (doc["duration_ms"].is<long>()) {
        long d = doc["duration_ms"].as<long>();
        if (d < 1000 || d > 60000) { _sendError(req, 400, "duration_ms must be 1000-60000"); return; }
        alertDurationMs = (uint32_t)d;
    }

    // Optional: temporary brightness/scroll-speed override, active only while
    // this alert is on screen — restored to the configured value afterward.
    alertBrightness = -1;
    if (doc["brightness"].is<int>()) {
        int v = doc["brightness"].as<int>();
        if (v < 0 || v > 15) { _sendError(req, 400, "brightness must be 0-15"); return; }
        alertBrightness = (int16_t)v;
    }
    alertScrollSpeedMs = -1;
    if (doc["scroll_speed_ms"].is<int>()) {
        int v = doc["scroll_speed_ms"].as<int>();
        if (v < 10 || v > 200) { _sendError(req, 400, "scroll_speed_ms must be 10-200"); return; }
        alertScrollSpeedMs = v;
    }

    // Append to alert history ring buffer (newest entry overwrites oldest when full)
    {
        uint8_t idx;
        if (alertHistoryCount < ALERT_HISTORY_SIZE) {
            idx = alertHistoryCount;
            alertHistoryCount++;
        } else {
            // Buffer full — overwrite the oldest entry and advance the head
            idx = alertHistoryHead;
            alertHistoryHead = (alertHistoryHead + 1) % ALERT_HISTORY_SIZE;
        }
        alertHistory[idx].timestamp = time(nullptr);
        strncpy(alertHistory[idx].message, alertMessage, MAX_ALERT_LEN - 1);
        alertHistory[idx].message[MAX_ALERT_LEN - 1] = '\0';
    }

    alertPending = true;
    _sendOk(req);
}

// ─── GET /api/alerts/history ─────────────────────────────────────────────────

static void _handleGetAlertsHistory(AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    // Walk the ring buffer from oldest to newest
    for (uint8_t i = 0; i < alertHistoryCount; i++) {
        uint8_t idx = (alertHistoryHead + i) % ALERT_HISTORY_SIZE;
        JsonObject entry = arr.add<JsonObject>();
        entry["timestamp"] = (long long)alertHistory[idx].timestamp;
        entry["message"]   = alertHistory[idx].message;
    }

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ─── GET /api/timezones ───────────────────────────────────────────────────────

static void _handleGetTimezones(AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    uint8_t count = tzTableSize();
    for (uint8_t i = 0; i < count; i++) {
        const TZEntry* e = tzTableEntry(i);
        if (e) arr.add(e->iana);
    }

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ─── POST /api/wifi ───────────────────────────────────────────────────────────

static void _handlePostWifi(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                            size_t index, size_t total) {
    (void)index; (void)total;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) { _sendError(req, 400, "Invalid JSON"); return; }

    if (!doc["ssid"].is<const char*>()) {
        _sendError(req, 400, "Missing 'ssid' field"); return;
    }

    const char* ssid = doc["ssid"].as<const char*>();
    const char* pass = doc["password"].is<const char*>() ? doc["password"].as<const char*>() : "";

    if (strlen(ssid) == 0 || strlen(ssid) >= WIFI_SSID_MAX) {
        _sendError(req, 400, "ssid invalid length"); return;
    }
    if (strlen(pass) >= WIFI_PASS_MAX) {
        _sendError(req, 400, "password too long"); return;
    }

    strncpy(cfgWifiSsid, ssid, WIFI_SSID_MAX - 1);
    cfgWifiSsid[WIFI_SSID_MAX - 1] = '\0';
    strncpy(cfgWifiPass, pass, WIFI_PASS_MAX - 1);
    cfgWifiPass[WIFI_PASS_MAX - 1] = '\0';
    saveConfig();

    scheduleRestart(RESTART_DELAY_MS);
    _sendOk(req);
}

// ─── POST /api/preview ───────────────────────────────────────────────────────
// Force-show a slot on the display immediately, bypassing the rotation timer.
// Body: {"slot": 2}   (2 = Weather; future slots follow the same pattern)
// The slot must be enabled and have a valid cache; otherwise returns ok:false.

static void _handlePostPreview(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                                size_t index, size_t total) {
    (void)index; (void)total;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) { _sendError(req, 400, "Invalid JSON"); return; }

    if (!doc["slot"].is<int>()) { _sendError(req, 400, "Missing 'slot' field"); return; }
    int slot = doc["slot"].as<int>();
    if (slot < 2 || slot > 3) { _sendError(req, 400, "slot must be 2 (weather) or 3 (quotes)"); return; }

    if (slot == 2) {
        if (!slotEnabled[2])        { _sendError(req, 400, "Weather slot is disabled"); return; }
        if (!weatherCache.valid)    { _sendError(req, 400, "No weather data cached yet"); return; }
    } else if (slot == 3) {
        if (!slotEnabled[3])        { _sendError(req, 400, "Quotes slot is disabled"); return; }
        if (quoteCacheCount == 0)   { _sendError(req, 400, "No quotes data cached yet"); return; }
    }

    displayForceSlot((uint8_t)slot);
    _sendOk(req);
}

// ─── webRoutesBegin ───────────────────────────────────────────────────────────

void webRoutesBegin(AsyncWebServer& server) {
    server.on("/", HTTP_GET, _handleRoot);

    server.on("/api/status",          HTTP_GET,  _handleGetStatus);
    server.on("/api/config",          HTTP_GET,  _handleGetConfig);
    server.on("/api/timezones",       HTTP_GET,  _handleGetTimezones);
    server.on("/api/alerts/history",  HTTP_GET,  _handleGetAlertsHistory);

    // Body-receiving handlers (POST)
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostConfig);

    server.on("/api/alert", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostAlert);

    server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostWifi);

    server.on("/api/preview", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostPreview);

    // 404 handler
    server.onNotFound([](AsyncWebServerRequest* req){
        req->send(404, "application/json", "{\"ok\":false,\"error\":\"Not found\"}");
    });
}
