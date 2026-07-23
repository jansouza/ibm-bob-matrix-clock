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
#include "data_fetcher.h"

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

    slotIntervalMs[2] = _prefs.getUInt(NVS_KEY_SLOT2_MS, WEATHER_DISPLAY_DEFAULT_MS);
    slotIntervalMs[3] = _prefs.getUInt(NVS_KEY_SLOT3_MS, QUOTES_DISPLAY_DEFAULT_MS);

    cfgDateIntervalMs  = _prefs.getUInt(NVS_KEY_DATE_INT_MS, DATE_INTERVAL_DEFAULT_MS);
    cfgDateEnabled     = _prefs.getBool(NVS_KEY_DATE_EN, true);

    _prefs.getString(NVS_KEY_UI_LANGUAGE, cfgUiLanguage, UI_LANG_CODE_MAX);
    if (cfgUiLanguage[0] == '\0' || !isUiLanguageValid(cfgUiLanguage)) {
        strncpy(cfgUiLanguage, UI_LANG_DEFAULT, UI_LANG_CODE_MAX - 1);
        cfgUiLanguage[UI_LANG_CODE_MAX - 1] = '\0';
    }

    // ── Weather (Phase 4) ──────────────────────────────────────────────────────
    slotEnabled[2]     = _prefs.getBool(NVS_KEY_WEATHER_EN,   false);
    cfgWeatherUpdateMs = _prefs.getUInt(NVS_KEY_WEATHER_UPMS, WEATHER_UPDATE_DEFAULT_MS);
    cfgWeatherLat      = _prefs.getFloat(NVS_KEY_WEATHER_LAT, WEATHER_LAT_DEFAULT);
    cfgWeatherLon      = _prefs.getFloat(NVS_KEY_WEATHER_LON, WEATHER_LON_DEFAULT);
    _prefs.getString(NVS_KEY_TEMP_UNIT, cfgTempUnit, WEATHER_TEMP_UNIT_MAX);
    if (cfgTempUnit[0] == '\0') strncpy(cfgTempUnit, WEATHER_TEMP_UNIT_DEFAULT, WEATHER_TEMP_UNIT_MAX - 1);

    // ── Quotes (Phase 5) ────────────────────────────────────────────────────────
    slotEnabled[3]    = _prefs.getBool(NVS_KEY_QUOTES_EN,   false);
    cfgQuotesUpdateMs = _prefs.getUInt(NVS_KEY_QUOTES_UPMS, QUOTES_UPDATE_DEFAULT_MS);
    _prefs.getString(NVS_KEY_QUOTES_TICKERS, cfgQuotesTickers, QUOTES_TICKERS_MAX);

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

    // ── Weather (Phase 4) ──────────────────────────────────────────────────────
    _prefs.putBool(NVS_KEY_WEATHER_EN,   slotEnabled[2]);
    _prefs.putUInt(NVS_KEY_WEATHER_UPMS, cfgWeatherUpdateMs);
    _prefs.putFloat(NVS_KEY_WEATHER_LAT, cfgWeatherLat);
    _prefs.putFloat(NVS_KEY_WEATHER_LON, cfgWeatherLon);
    _prefs.putString(NVS_KEY_TEMP_UNIT,  cfgTempUnit);

    // ── Quotes (Phase 5) ────────────────────────────────────────────────────────
    _prefs.putBool(NVS_KEY_QUOTES_EN,   slotEnabled[3]);
    _prefs.putUInt(NVS_KEY_QUOTES_UPMS, cfgQuotesUpdateMs);
    _prefs.putString(NVS_KEY_QUOTES_TICKERS, cfgQuotesTickers);

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

    slotIntervalMs[2] = WEATHER_DISPLAY_DEFAULT_MS;
    slotIntervalMs[3] = QUOTES_DISPLAY_DEFAULT_MS;

    cfgDateIntervalMs = DATE_INTERVAL_DEFAULT_MS;
    cfgDateEnabled    = true;

    strncpy(cfgUiLanguage, UI_LANG_DEFAULT, UI_LANG_CODE_MAX - 1);
    cfgUiLanguage[UI_LANG_CODE_MAX - 1] = '\0';

    // ── Weather (Phase 4) ──────────────────────────────────────────────────────
    cfgWeatherLat      = WEATHER_LAT_DEFAULT;
    cfgWeatherLon      = WEATHER_LON_DEFAULT;
    cfgWeatherUpdateMs = WEATHER_UPDATE_DEFAULT_MS;
    strncpy(cfgTempUnit, WEATHER_TEMP_UNIT_DEFAULT, WEATHER_TEMP_UNIT_MAX - 1);
    cfgTempUnit[WEATHER_TEMP_UNIT_MAX - 1] = '\0';
    weatherCache = { 0.0f, 0.0f, 0.0f, {0}, false, false, 0 };

    // ── Quotes (Phase 5) ────────────────────────────────────────────────────────
    cfgQuotesUpdateMs = QUOTES_UPDATE_DEFAULT_MS;
    cfgQuotesTickers[0] = '\0';
    for (uint8_t i = 0; i < QUOTES_MAX_TICKERS; i++) quoteCache[i] = { {0}, 0.0f, 0.0f, false };
    quoteCacheCount  = 0;
    quotesCacheStale = false;
}
