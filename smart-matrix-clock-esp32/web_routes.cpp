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

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

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

// Validate API key header when authentication is enabled.
// Returns true if the request should be allowed, false if rejected (response already sent).
static bool _checkApiKey(AsyncWebServerRequest* req) {
    if (!cfgApiAuthEnabled) return true;
    if (req->hasHeader(API_KEY_HEADER)) {
        const String& hdr = req->header(API_KEY_HEADER);
        if (hdr == cfgApiKey) return true;
    }
    _sendError(req, 401, "Unauthorized: invalid or missing API key");
    return false;
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
    doc["api_auth_enabled"] = cfgApiAuthEnabled;
    // api_key is intentionally omitted from GET /api/config — use the API tab in the web UI

    doc["weather_enabled"]    = slotEnabled[2];
    doc["weather_update_ms"]  = slotIntervalMs[2];   // reused for fetch interval (Phase 4 will add separate field)
    doc["weather_display_ms"] = slotIntervalMs[2];
    doc["weather_lat"]        = 0.0;   // Phase 4 will populate from weatherCfgLat
    doc["weather_lon"]        = 0.0;
    doc["temp_unit"]          = "C";

    doc["quotes_enabled"]    = slotEnabled[3];
    doc["quotes_update_ms"]  = slotIntervalMs[3];
    doc["quotes_display_ms"] = slotIntervalMs[3];
    doc["quotes_tickers"]    = "";   // Phase 5 will populate

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ─── POST /api/config ────────────────────────────────────────────────────────

static void _handlePostConfig(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                              size_t index, size_t total) {
    // Accumulate body chunks (ESPAsyncWebServer calls this handler per chunk)
    (void)index; (void)total;

    if (!_checkApiKey(req)) return;

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

    // ── api_auth_enabled ──────────────────────────────────────────────────────
    if (doc["api_auth_enabled"].is<bool>()) {
        cfgApiAuthEnabled = doc["api_auth_enabled"].as<bool>();
        changed = true;
    }

    // ── regen_api_key ─────────────────────────────────────────────────────────
    if (doc["regen_api_key"].is<bool>() && doc["regen_api_key"].as<bool>()) {
        generateApiKey();
        changed = true;
        // Return new key in response
        JsonDocument resp;
        resp["ok"]      = true;
        resp["api_key"] = cfgApiKey;
        saveConfig();
        String body;
        serializeJson(resp, body);
        req->send(200, "application/json", body);
        return;
    }

    // ── weather_enabled ───────────────────────────────────────────────────────
    if (doc["weather_enabled"].is<bool>()) {
        slotEnabled[2] = doc["weather_enabled"].as<bool>();
        changed = true;
    }
    if (doc["weather_display_ms"].is<long>()) {
        long v = doc["weather_display_ms"].as<long>();
        if (v < 5000 || v > 300000) { _sendError(req, 400, "weather_display_ms out of range"); return; }
        slotIntervalMs[2] = (uint32_t)v;
        changed = true;
    }

    // ── quotes_enabled ────────────────────────────────────────────────────────
    if (doc["quotes_enabled"].is<bool>()) {
        slotEnabled[3] = doc["quotes_enabled"].as<bool>();
        changed = true;
    }
    if (doc["quotes_display_ms"].is<long>()) {
        long v = doc["quotes_display_ms"].as<long>();
        if (v < 5000 || v > 300000) { _sendError(req, 400, "quotes_display_ms out of range"); return; }
        slotIntervalMs[3] = (uint32_t)v;
        changed = true;
    }

    if (changed) saveConfig();
    _sendOk(req);
}

// ─── POST /api/alert ─────────────────────────────────────────────────────────

static void _handlePostAlert(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                             size_t index, size_t total) {
    (void)index; (void)total;

    if (!_checkApiKey(req)) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) { _sendError(req, 400, "Invalid JSON"); return; }

    if (!doc["message"].is<const char*>()) {
        _sendError(req, 400, "Missing 'message' field"); return;
    }

    const char* utf8msg = doc["message"].as<const char*>();
    if (strlen(utf8msg) == 0) { _sendError(req, 400, "message is empty"); return; }

    // Convert UTF-8 → Latin-1 before storing (display driver expects Latin-1)
    utf8ToLatin1(utf8msg, alertMessage, MAX_ALERT_LEN);

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

    alertPending = true;
    _sendOk(req);
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

    if (!_checkApiKey(req)) return;

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

// ─── webRoutesBegin ───────────────────────────────────────────────────────────

void webRoutesBegin(AsyncWebServer& server) {
    server.on("/", HTTP_GET, _handleRoot);

    server.on("/api/status",    HTTP_GET,  _handleGetStatus);
    server.on("/api/config",    HTTP_GET,  _handleGetConfig);
    server.on("/api/timezones", HTTP_GET,  _handleGetTimezones);

    // Body-receiving handlers (POST)
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostConfig);

    server.on("/api/alert", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostAlert);

    server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* req){},
              nullptr, _handlePostWifi);

    // 404 handler
    server.onNotFound([](AsyncWebServerRequest* req){
        req->send(404, "application/json", "{\"ok\":false,\"error\":\"Not found\"}");
    });
}
