/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include "globals.h"
#include "config.h"

// ─── Display settings ─────────────────────────────────────────────────────────
uint8_t  currentBrightness = DEFAULT_BRIGHTNESS;
uint16_t scrollSpeed        = DEFAULT_SCROLL_SPEED_MS;

// ─── NTP / time ───────────────────────────────────────────────────────────────
bool ntpSynced = false;

// ─── Alert message ────────────────────────────────────────────────────────────
bool alertPending    = false;
char alertMessage[MAX_ALERT_LEN] = {0};
uint8_t  alertMode       = ALERT_MODE_SCROLL;
uint32_t alertDurationMs = ALERT_DURATION_DEFAULT_MS;
int16_t  alertBrightness    = -1;   // -1 = no override, use currentBrightness
int32_t  alertScrollSpeedMs = -1;   // -1 = no override, use scrollSpeed

// ─── Slot rotation ────────────────────────────────────────────────────────────
uint8_t  activeSlot        = 0;
bool     slotEnabled[4]    = { true, false, false, false };   // [clock, alert, weather, quotes]
uint32_t slotIntervalMs[4] = { 0, 0, 60000, 120000 };         // clock = base (no interval)

// ─── Persistent configuration ─────────────────────────────────────────────────
char     cfgTimezone[NTP_TIMEZONE_MAX]  = NTP_TIMEZONE_DEFAULT;
char     cfgLanguage[LANG_CODE_MAX]     = LANG_DEFAULT;
char     cfgNtpServer[NTP_SERVER_MAX]   = NTP_SERVER_DEFAULT;
char     cfgWifiSsid[WIFI_SSID_MAX]     = {0};
char     cfgWifiPass[WIFI_PASS_MAX]     = {0};
uint32_t cfgDateIntervalMs              = DATE_INTERVAL_DEFAULT_MS;
bool     cfgDateEnabled                 = true;
char     cfgUiLanguage[UI_LANG_CODE_MAX] = UI_LANG_DEFAULT;

// ─── Weather configuration (Phase 4) ──────────────────────────────────────────
float    cfgWeatherLat     = WEATHER_LAT_DEFAULT;
float    cfgWeatherLon     = WEATHER_LON_DEFAULT;
uint32_t cfgWeatherUpdateMs = WEATHER_UPDATE_DEFAULT_MS;
char     cfgTempUnit[WEATHER_TEMP_UNIT_MAX] = WEATHER_TEMP_UNIT_DEFAULT;
WeatherCache weatherCache  = { 0.0f, 0.0f, 0.0f, {0}, false, false, 0 };

// ─── Quotes configuration (Phase 5) ────────────────────────────────────────────
uint32_t cfgQuotesUpdateMs = QUOTES_UPDATE_DEFAULT_MS;
char     cfgQuotesTickers[QUOTES_TICKERS_MAX] = {0};
QuoteCache quoteCache[QUOTES_MAX_TICKERS] = {};
uint8_t  quoteCacheCount   = 0;
bool     quotesCacheStale  = false;
