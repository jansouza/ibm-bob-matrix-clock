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
    // WEB_PAGE_HTML is ~62KB. The (code, contentType, const char*) overload
    // copies the whole body into a heap String first — on a fragmented heap
    // that single large allocation can silently fail, yielding a 200 OK with
    // Content-Length: 0 (blank page). This overload streams straight from the
    // static array in chunks instead, with no large intermediate allocation.
    req->send(200, "text/html", (const uint8_t*)WEB_PAGE_HTML, strlen(WEB_PAGE_HTML));
}

// ─── GET /api/status ─────────────────────────────────────────────────────────

static void _handleGetStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;

    doc["ntp_synced"]       = ntpSynced;
    doc["active_slot"]      = activeSlot;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["ssid"]             = WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "";
    doc["ip"]               = WiFi.status() == WL_CONNECTED
                                  ? WiFi.localIP().toString().c_str()
                                  : WiFi.softAPIP().toString().c_str();

    // Current time string for the live preview
    if (ntpSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo) && (timeinfo.tm_year + 1900) >= 2020) {
            char tbuf[10];
            if (cfgClockMode == CLOCK_MODE_HHMMSS) {
                snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d",
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            } else {
                snprintf(tbuf, sizeof(tbuf), "%02d:%02d",
                         timeinfo.tm_hour, timeinfo.tm_min);
            }
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
            char priceStr[24];
            formatQuotePrice(quoteCache[i].price, cfgLocale, priceStr, sizeof(priceStr));
            char entry[56];
            snprintf(entry, sizeof(entry), "%s%s: %s %c%.2f%%",
                     (i > 0) ? "  " : "",
                     quoteCache[i].symbol,
                     priceStr,
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
    doc["locale"]           = cfgLocale;
    doc["ntp_server"]       = cfgNtpServer;
    doc["date_interval_ms"] = cfgDateIntervalMs;
    doc["date_enabled"]     = cfgDateEnabled;
    doc["ui_language"]      = cfgUiLanguage;
    doc["clock_mode"]       = cfgClockMode;

    doc["night_brightness_enabled"] = cfgNightBrightnessEnabled;
    doc["night_brightness_level"]   = cfgNightBrightnessLevel;
    doc["night_start_min"]          = cfgNightStartMin;
    doc["night_end_min"]            = cfgNightEndMin;

    doc["weather_enabled"]    = slotEnabled[2];
    doc["weather_update_ms"]  = cfgWeatherUpdateMs;
    doc["weather_display_ms"] = slotIntervalMs[2];
    doc["weather_lat"]        = cfgWeatherLat;
    doc["weather_lon"]        = cfgWeatherLon;
    doc["temp_unit"]          = cfgTempUnit;
    doc["weather_sched_start"] = slotScheduleStartMin[2];
    doc["weather_sched_end"]   = slotScheduleEndMin[2];
    doc["weather_sched_days"]  = slotScheduleDaysMask[2];

    doc["quotes_enabled"]    = slotEnabled[3];
    doc["quotes_update_ms"]  = cfgQuotesUpdateMs;
    doc["quotes_display_ms"] = slotIntervalMs[3];
    doc["quotes_tickers"]    = cfgQuotesTickers;
    doc["quotes_sched_start"] = slotScheduleStartMin[3];
    doc["quotes_sched_end"]   = slotScheduleEndMin[3];
    doc["quotes_sched_days"]  = slotScheduleDaysMask[3];

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

    // ── locale ────────────────────────────────────────────────────────────────
    if (doc["locale"].is<const char*>()) {
        const char* locale = doc["locale"].as<const char*>();
        if (strcmp(locale, "pt") != 0 && strcmp(locale, "en") != 0) {
            _sendError(req, 400, "locale must be 'pt' or 'en'"); return;
        }
        strncpy(cfgLocale, locale, LOCALE_CODE_MAX - 1);
        cfgLocale[LOCALE_CODE_MAX - 1] = '\0';
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

    // ── clock_mode ────────────────────────────────────────────────────────────
    if (doc["clock_mode"].is<int>()) {
        int v = doc["clock_mode"].as<int>();
        if (v < 0 || v > 1) { _sendError(req, 400, "clock_mode must be 0 (HH:MM) or 1 (HH:MM:SS)"); return; }
        cfgClockMode = (uint8_t)v;
        clockModeChangePending = true;
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
    if (doc["weather_sched_start"].is<int>() && doc["weather_sched_end"].is<int>()) {
        int s = doc["weather_sched_start"].as<int>();
        int e = doc["weather_sched_end"].as<int>();
        if (s < SLOT_SCHEDULE_MIN_MINUTE || s > SLOT_SCHEDULE_MAX_MINUTE ||
            e < SLOT_SCHEDULE_MIN_MINUTE || e > SLOT_SCHEDULE_MAX_MINUTE) {
            _sendError(req, 400, "weather_sched_start/end must be 0-1439"); return;
        }
        slotScheduleStartMin[2] = (uint16_t)s;
        slotScheduleEndMin[2]   = (uint16_t)e;
        changed = true;
    }
    if (doc["weather_sched_days"].is<int>()) {
        int d = doc["weather_sched_days"].as<int>();
        if (d < 0 || d > SLOT_SCHEDULE_DAYS_MAX) { _sendError(req, 400, "weather_sched_days must be 0-127"); return; }
        slotScheduleDaysMask[2] = (uint8_t)d;
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
    if (doc["quotes_sched_start"].is<int>() && doc["quotes_sched_end"].is<int>()) {
        int s = doc["quotes_sched_start"].as<int>();
        int e = doc["quotes_sched_end"].as<int>();
        if (s < SLOT_SCHEDULE_MIN_MINUTE || s > SLOT_SCHEDULE_MAX_MINUTE ||
            e < SLOT_SCHEDULE_MIN_MINUTE || e > SLOT_SCHEDULE_MAX_MINUTE) {
            _sendError(req, 400, "quotes_sched_start/end must be 0-1439"); return;
        }
        slotScheduleStartMin[3] = (uint16_t)s;
        slotScheduleEndMin[3]   = (uint16_t)e;
        changed = true;
    }
    if (doc["quotes_sched_days"].is<int>()) {
        int d = doc["quotes_sched_days"].as<int>();
        if (d < 0 || d > SLOT_SCHEDULE_DAYS_MAX) { _sendError(req, 400, "quotes_sched_days must be 0-127"); return; }
        slotScheduleDaysMask[3] = (uint8_t)d;
        changed = true;
    }

    // ── night_brightness_enabled ──────────────────────────────────────────────
    if (doc["night_brightness_enabled"].is<bool>()) {
        cfgNightBrightnessEnabled = doc["night_brightness_enabled"].as<bool>();
        changed = true;
    }
    if (doc["night_brightness_level"].is<int>()) {
        int v = doc["night_brightness_level"].as<int>();
        if (v < 0 || v > 15) { _sendError(req, 400, "night_brightness_level must be 0-15"); return; }
        cfgNightBrightnessLevel = (uint8_t)v;
        changed = true;
    }
    if (doc["night_start_min"].is<int>()) {
        int v = doc["night_start_min"].as<int>();
        if (v < 0 || v > 1439) { _sendError(req, 400, "night_start_min must be 0-1439"); return; }
        cfgNightStartMin = (uint16_t)v;
        changed = true;
    }
    if (doc["night_end_min"].is<int>()) {
        int v = doc["night_end_min"].as<int>();
        if (v < 0 || v > 1439) { _sendError(req, 400, "night_end_min must be 0-1439"); return; }
        cfgNightEndMin = (uint16_t)v;
        changed = true;
    }

    if (changed) saveConfig();
    _sendOk(req);
}

// ─── POST /api/message ─────────────────────────────────────────────────────────

static void _handlePostMessage(AsyncWebServerRequest* req, uint8_t* data, size_t len,
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
    char latin1msg[MAX_MESSAGE_LEN];
    utf8ToLatin1(utf8msg, latin1msg, MAX_MESSAGE_LEN);
    expandIconTags(latin1msg, messageText, MAX_MESSAGE_LEN);

    // Optional: mode and duration (only relevant for non-scroll modes)
    if (doc["mode"].is<int>()) {
        int m = doc["mode"].as<int>();
        if (m < 0 || m > 3) { _sendError(req, 400, "mode must be 0-3"); return; }
        messageMode = (uint8_t)m;
    }
    if (doc["duration_ms"].is<long>()) {
        long d = doc["duration_ms"].as<long>();
        if (d < 1000 || d > 60000) { _sendError(req, 400, "duration_ms must be 1000-60000"); return; }
        messageDurationMs = (uint32_t)d;
    }

    // Optional: temporary brightness/scroll-speed override, active only while
    // this message is on screen — restored to the configured value afterward.
    messageBrightness = -1;
    if (doc["brightness"].is<int>()) {
        int v = doc["brightness"].as<int>();
        if (v < 0 || v > 15) { _sendError(req, 400, "brightness must be 0-15"); return; }
        messageBrightness = (int16_t)v;
    }
    messageScrollSpeedMs = -1;
    if (doc["scroll_speed_ms"].is<int>()) {
        int v = doc["scroll_speed_ms"].as<int>();
        if (v < 10 || v > 200) { _sendError(req, 400, "scroll_speed_ms must be 10-200"); return; }
        messageScrollSpeedMs = v;
    }

    // Append to message history ring buffer (newest entry overwrites oldest when full)
    {
        uint8_t idx;
        if (messageHistoryCount < MESSAGE_HISTORY_SIZE) {
            idx = messageHistoryCount;
            messageHistoryCount++;
        } else {
            // Buffer full — overwrite the oldest entry and advance the head
            idx = messageHistoryHead;
            messageHistoryHead = (messageHistoryHead + 1) % MESSAGE_HISTORY_SIZE;
        }
        messageHistory[idx].timestamp = time(nullptr);
        strncpy(messageHistory[idx].message, utf8msg, MAX_MESSAGE_LEN - 1);
        messageHistory[idx].message[MAX_MESSAGE_LEN - 1] = '\0';
    }

    messagePending = true;
    _sendOk(req);
}

// ─── GET /api/messages/history ─────────────────────────────────────────────────

static void _handleGetMessagesHistory(AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    // Walk the ring buffer from oldest to newest
    for (uint8_t i = 0; i < messageHistoryCount; i++) {
        uint8_t idx = (messageHistoryHead + i) % MESSAGE_HISTORY_SIZE;
        JsonObject entry = arr.add<JsonObject>();
        entry["timestamp"] = (long long)messageHistory[idx].timestamp;
        entry["message"]   = messageHistory[idx].message;
    }

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ─── GET /api/wifi/scan ──────────────────────────────────────────────────────
// Triggers a synchronous WiFi scan and returns all visible networks as a JSON
// array of { ssid, rssi, secure } objects, sorted by signal strength (desc).
// Intended for the web setup panel — user-triggered, so the ~2 s blocking scan
// is acceptable here (same rationale as the blocking delay in _stationConnect).

static void _handleGetWifiScan(AsyncWebServerRequest* req) {
    int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/false);

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    if (n > 0) {
        for (int i = 0; i < n; i++) {
            JsonObject net = arr.add<JsonObject>();
            net["ssid"]   = WiFi.SSID(i).c_str();
            net["rssi"]   = WiFi.RSSI(i);
            net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        }
    }

    WiFi.scanDelete();  // free scan result memory

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

// ─── POST /api/fetch ────────────────────────────────────────────────────────
// Force an immediate re-fetch of a slot's data, bypassing its update interval.
// Body: {"slot": 2}   (2 = Weather, 3 = Quotes)
// Only flags the fetch for the next fetcherTick() — the actual HTTPClient call
// happens there, never in this handler (see AGENTS.md: HTTP handlers never do I/O).
// The slot must be enabled; otherwise returns ok:false.

static void _handlePostFetch(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                              size_t index, size_t total) {
    (void)index; (void)total;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) { _sendError(req, 400, "Invalid JSON"); return; }

    if (!doc["slot"].is<int>()) { _sendError(req, 400, "Missing 'slot' field"); return; }
    int slot = doc["slot"].as<int>();
    if (slot < 2 || slot > 3) { _sendError(req, 400, "slot must be 2 (weather) or 3 (quotes)"); return; }

    if (slot == 2) {
        if (!slotEnabled[2]) { _sendError(req, 400, "Weather slot is disabled"); return; }
        fetcherReset();
    } else {
        if (!slotEnabled[3]) { _sendError(req, 400, "Quotes slot is disabled"); return; }
        quotesFetcherReset();
    }

    _sendOk(req);
}

// ─── webRoutesBegin ───────────────────────────────────────────────────────────

void webRoutesBegin(AsyncWebServer& server) {
    server.on("/", HTTP_GET, _handleRoot);

    server.on("/api/status",          HTTP_GET,  _handleGetStatus);
    server.on("/api/config",          HTTP_GET,  _handleGetConfig);
    server.on("/api/timezones",       HTTP_GET,  _handleGetTimezones);
    server.on("/api/messages/history",  HTTP_GET,  _handleGetMessagesHistory);
    server.on("/api/wifi/scan",       HTTP_GET,  _handleGetWifiScan);

    // Body-receiving handlers (POST)
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostConfig);

    server.on("/api/message", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostMessage);

    server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostWifi);

    server.on("/api/preview", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostPreview);

    server.on("/api/fetch", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostFetch);

    // 404 handler
    server.onNotFound([](AsyncWebServerRequest* req){
        req->send(404, "application/json", "{\"ok\":false,\"error\":\"Not found\"}");
    });
}
