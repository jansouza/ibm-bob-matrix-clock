#include "wifi_manager.h"
#include "config.h"
#include "globals.h"

#include <WiFi.h>
#include <Arduino.h>

// ─── Internal state ───────────────────────────────────────────────────────────

static bool     _apMode          = false;
static uint32_t _lastReconnect   = 0;
static bool     _restartPending  = false;
static uint32_t _restartAt       = 0;

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Attempt station connect. Returns true if connected within WIFI_TIMEOUT_MS.
static bool _stationConnect(const char* ssid, const char* pass) {
    if (!ssid || ssid[0] == '\0') return false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= WIFI_TIMEOUT_MS) {
            WiFi.disconnect(true);
            return false;
        }
        delay(200);   // blocking — only called during setup()
    }
    return true;
}

// Bring up the configuration Access Point.
static void _startConfigAP() {
    _apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, (WIFI_AP_PASSWORD[0] == '\0') ? nullptr : WIFI_AP_PASSWORD);
    IPAddress ip;
    ip.fromString(WIFI_AP_IP);
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));

    Serial.printf("[WiFi] AP mode — SSID: %s  IP: %s\n",
                  WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
}

// ─── Public API ───────────────────────────────────────────────────────────────

void wifiBegin() {
    bool connected = _stationConnect(cfgWifiSsid, cfgWifiPass);
    if (connected) {
        _apMode = false;
        Serial.printf("[WiFi] Connected — IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[WiFi] No saved credentials or timeout — starting setup AP");
        _startConfigAP();
    }
}

void wifiTick() {
    uint32_t now = millis();

    // ── Deferred restart ──────────────────────────────────────────────────────
    if (_restartPending && now >= _restartAt) {
        _restartPending = false;
        ESP.restart();
    }

    // ── Skip reconnect logic when in AP mode ─────────────────────────────────
    if (_apMode) return;

    // ── Reconnect if station dropped ──────────────────────────────────────────
    if (WiFi.status() != WL_CONNECTED) {
        if (now - _lastReconnect >= WIFI_RECONNECT_MS) {
            _lastReconnect = now;
            Serial.println("[WiFi] Disconnected — attempting reconnect…");
            WiFi.disconnect(true);
            WiFi.begin(cfgWifiSsid, cfgWifiPass);
        }
    }
}

void scheduleRestart(uint32_t delayMs) {
    _restartPending = true;
    _restartAt      = millis() + delayMs;
}

bool isRestartPending() {
    return _restartPending;
}

bool isApMode() {
    return _apMode;
}
