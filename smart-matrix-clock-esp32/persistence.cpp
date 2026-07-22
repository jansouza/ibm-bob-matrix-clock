/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include "persistence.h"
#include "config.h"
#include "globals.h"
#include "locale_data.h"

#include <Preferences.h>
#include <Arduino.h>

// ─── Internal helpers ─────────────────────────────────────────────────────────

static Preferences _prefs;

// Open namespace for reading
static void _openR() { _prefs.begin(NVS_NAMESPACE, /*readOnly=*/true); }

// Open namespace for writing
static void _openW() { _prefs.begin(NVS_NAMESPACE, /*readOnly=*/false); }

static void _close() { _prefs.end(); }

// Single source of truth for accepted web UI language codes.
// Add a new entry here (and a matching I18N dictionary in web_page.cpp)
// to support another language.
static const char* const _uiLanguages[] = { "en", "pt" };
static const uint8_t _uiLanguagesCount = sizeof(_uiLanguages) / sizeof(_uiLanguages[0]);

bool isUiLanguageValid(const char* code) {
    for (uint8_t i = 0; i < _uiLanguagesCount; i++) {
        if (strcmp(code, _uiLanguages[i]) == 0) return true;
    }
    return false;
}

// ─── loadConfig ───────────────────────────────────────────────────────────────

void loadConfig() {
    _openR();

    currentBrightness = _prefs.getUChar(NVS_KEY_BRIGHTNESS, DEFAULT_BRIGHTNESS);
    scrollSpeed       = _prefs.getUShort(NVS_KEY_SCROLL_SPD, DEFAULT_SCROLL_SPEED_MS);

    _prefs.getString(NVS_KEY_TIMEZONE, cfgTimezone, NTP_TIMEZONE_MAX);
    if (cfgTimezone[0] == '\0') strncpy(cfgTimezone, NTP_TIMEZONE_DEFAULT, NTP_TIMEZONE_MAX - 1);

    _prefs.getString(NVS_KEY_LANGUAGE, cfgLanguage, LANG_CODE_MAX);
    if (cfgLanguage[0] == '\0') strncpy(cfgLanguage, LANG_DEFAULT, LANG_CODE_MAX - 1);

    _prefs.getString(NVS_KEY_NTP_SERVER, cfgNtpServer, NTP_SERVER_MAX);
    if (cfgNtpServer[0] == '\0') strncpy(cfgNtpServer, NTP_SERVER_DEFAULT, NTP_SERVER_MAX - 1);

    _prefs.getString(NVS_KEY_WIFI_SSID, cfgWifiSsid, WIFI_SSID_MAX);
    _prefs.getString(NVS_KEY_WIFI_PASS, cfgWifiPass, WIFI_PASS_MAX);

    slotEnabled[0] = _prefs.getBool(NVS_KEY_SLOT0_EN, true);
    slotEnabled[1] = _prefs.getBool(NVS_KEY_SLOT1_EN, false);
    slotEnabled[2] = _prefs.getBool(NVS_KEY_SLOT2_EN, false);
    slotEnabled[3] = _prefs.getBool(NVS_KEY_SLOT3_EN, false);

    slotIntervalMs[2] = _prefs.getUInt(NVS_KEY_SLOT2_MS, 30000);
    slotIntervalMs[3] = _prefs.getUInt(NVS_KEY_SLOT3_MS, 30000);

    cfgDateIntervalMs  = _prefs.getUInt(NVS_KEY_DATE_INT_MS, DATE_INTERVAL_DEFAULT_MS);
    cfgDateEnabled     = _prefs.getBool(NVS_KEY_DATE_EN, true);

    _prefs.getString(NVS_KEY_UI_LANGUAGE, cfgUiLanguage, UI_LANG_CODE_MAX);
    if (cfgUiLanguage[0] == '\0' || !isUiLanguageValid(cfgUiLanguage)) {
        strncpy(cfgUiLanguage, UI_LANG_DEFAULT, UI_LANG_CODE_MAX - 1);
        cfgUiLanguage[UI_LANG_CODE_MAX - 1] = '\0';
    }

    _close();
}

// ─── saveConfig ───────────────────────────────────────────────────────────────

void saveConfig() {
    _openW();

    _prefs.putUChar(NVS_KEY_BRIGHTNESS, currentBrightness);
    _prefs.putUShort(NVS_KEY_SCROLL_SPD, scrollSpeed);

    _prefs.putString(NVS_KEY_TIMEZONE,   cfgTimezone);
    _prefs.putString(NVS_KEY_LANGUAGE,   cfgLanguage);
    _prefs.putString(NVS_KEY_NTP_SERVER, cfgNtpServer);
    _prefs.putString(NVS_KEY_WIFI_SSID,  cfgWifiSsid);
    _prefs.putString(NVS_KEY_WIFI_PASS,  cfgWifiPass);

    _prefs.putBool(NVS_KEY_SLOT0_EN, slotEnabled[0]);
    _prefs.putBool(NVS_KEY_SLOT1_EN, slotEnabled[1]);
    _prefs.putBool(NVS_KEY_SLOT2_EN, slotEnabled[2]);
    _prefs.putBool(NVS_KEY_SLOT3_EN, slotEnabled[3]);

    _prefs.putUInt(NVS_KEY_SLOT2_MS, slotIntervalMs[2]);
    _prefs.putUInt(NVS_KEY_SLOT3_MS, slotIntervalMs[3]);

    _prefs.putUInt(NVS_KEY_DATE_INT_MS, cfgDateIntervalMs);
    _prefs.putBool(NVS_KEY_DATE_EN,     cfgDateEnabled);

    _prefs.putString(NVS_KEY_UI_LANGUAGE, cfgUiLanguage);

    _close();
}

// ─── applyTimezone ────────────────────────────────────────────────────────────

void applyTimezone() {
    const char* posix = ianaToPostfix(cfgTimezone);
    if (posix == nullptr) {
        // Fall back to Sao Paulo POSIX if unknown
        posix = ianaToPostfix(NTP_TIMEZONE_DEFAULT);
    }
    setenv("TZ", posix, 1);
    tzset();
}

// ─── factoryReset ─────────────────────────────────────────────────────────────

void factoryReset() {
    _openW();
    _prefs.clear();   // erase all keys in the namespace
    _close();

    // Reset RAM globals to compile-time defaults
    currentBrightness = DEFAULT_BRIGHTNESS;
    scrollSpeed       = DEFAULT_SCROLL_SPEED_MS;

    strncpy(cfgTimezone,  NTP_TIMEZONE_DEFAULT, NTP_TIMEZONE_MAX - 1);
    strncpy(cfgLanguage,  LANG_DEFAULT,         LANG_CODE_MAX - 1);
    strncpy(cfgNtpServer, NTP_SERVER_DEFAULT,   NTP_SERVER_MAX - 1);
    cfgWifiSsid[0] = '\0';
    cfgWifiPass[0] = '\0';

    slotEnabled[0] = true;
    slotEnabled[1] = false;
    slotEnabled[2] = false;
    slotEnabled[3] = false;

    slotIntervalMs[2] = 30000;
    slotIntervalMs[3] = 30000;

    cfgDateIntervalMs = DATE_INTERVAL_DEFAULT_MS;
    cfgDateEnabled    = true;

    strncpy(cfgUiLanguage, UI_LANG_DEFAULT, UI_LANG_CODE_MAX - 1);
    cfgUiLanguage[UI_LANG_CODE_MAX - 1] = '\0';
}
