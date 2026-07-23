#pragma once

/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// ─── Weather cache (Phase 4) ──────────────────────────────────────────────────
struct WeatherCache {
    float    temp;                          // current temperature
    float    minTemp;                       // day's min temperature
    float    maxTemp;                       // day's max temperature
    char     condition[WEATHER_CONDITION_MAX]; // localised condition string (Latin-1)
    bool     valid;                         // true once a successful fetch occurred
    bool     stale;                         // true if last fetch failed but cache exists
    uint32_t fetchedAt;                     // millis() of last successful fetch
};

// ─── Quotes cache (Phase 5) ───────────────────────────────────────────────────
struct QuoteCache {
    char     symbol[QUOTES_SYMBOL_MAX];     // e.g. "PETR4.SA"
    float    price;                         // regularMarketPrice
    float    changePercent;                 // regularMarketChangePercent
    bool     valid;                         // true once a successful fetch occurred
};

// ─── Alert history ────────────────────────────────────────────────────────────
struct AlertHistoryEntry {
    time_t  timestamp;                      // Unix time when the alert was received
    char    message[MAX_ALERT_LEN];         // Latin-1 encoded message (as stored)
};

// ─── Display settings ─────────────────────────────────────────────────────────
extern uint8_t  currentBrightness;    // 0–15
extern uint16_t scrollSpeed;          // ms per scroll frame

// ─── NTP / time ───────────────────────────────────────────────────────────────
extern bool     ntpSynced;            // true once first NTP sync is done

// ─── Alert message ────────────────────────────────────────────────────────────
extern bool     alertPending;         // set by HTTP handler, cleared by display
extern char     alertMessage[];       // Latin-1 encoded, null-terminated
extern uint8_t  alertMode;            // ALERT_MODE_SCROLL / BLINK / STATIC
extern uint32_t alertDurationMs;      // duration for blink/static modes (ms)
extern int16_t  alertBrightness;      // per-alert brightness override, 0–15; -1 = use currentBrightness
extern int32_t  alertScrollSpeedMs;   // per-alert scroll speed override, ms; -1 = use scrollSpeed
extern AlertHistoryEntry alertHistory[];  // ring buffer of recent alerts
extern uint8_t  alertHistoryCount;    // number of valid entries (0..ALERT_HISTORY_SIZE)
extern uint8_t  alertHistoryHead;     // index of the oldest entry (ring buffer head)

// ─── Slot rotation ────────────────────────────────────────────────────────────
extern uint8_t  activeSlot;           // index of the currently active slot
extern bool     slotEnabled[];        // whether each slot is enabled
extern uint32_t slotIntervalMs[];     // how long each slot is displayed (ms)

// ─── Persistent configuration (loaded at boot, saved on change) ───────────────
extern char     cfgTimezone[];        // IANA timezone name (e.g. "America/Sao_Paulo")
extern char     cfgLanguage[];        // "pt" or "en"
extern char     cfgNtpServer[];       // NTP server hostname
extern char     cfgWifiSsid[];        // stored WiFi SSID (may be empty)
extern char     cfgWifiPass[];        // stored WiFi password
extern uint32_t cfgDateIntervalMs;    // how often date fires (ms); 0 = disabled
extern bool     cfgDateEnabled;       // true = show date periodically
extern char     cfgUiLanguage[];      // web panel UI language (e.g. "en", "pt") — independent of cfgLanguage

// ─── Weather configuration (Phase 4) ──────────────────────────────────────────
extern float    cfgWeatherLat;        // latitude
extern float    cfgWeatherLon;        // longitude
extern uint32_t cfgWeatherUpdateMs;   // fetch interval (ms)
extern char     cfgTempUnit[];        // "C" or "F"
extern WeatherCache weatherCache;     // last known weather data

// ─── Quotes configuration (Phase 5) ────────────────────────────────────────────
extern uint32_t cfgQuotesUpdateMs;                    // fetch interval (ms)
extern char     cfgQuotesTickers[];                   // raw comma-separated tickers string
extern QuoteCache quoteCache[QUOTES_MAX_TICKERS];     // last known quote data
extern uint8_t  quoteCacheCount;                      // number of symbols currently tracked
extern bool     quotesCacheStale;                     // true if last fetch failed but cache exists
