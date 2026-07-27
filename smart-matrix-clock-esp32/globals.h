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
#include <time.h>

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

// ─── Message history ──────────────────────────────────────────────────────────
struct MessageHistoryEntry {
    time_t  timestamp;                      // Unix time when the message was received
    char    message[MAX_MESSAGE_LEN];       // UTF-8 encoded message, as originally submitted
};

// ─── Display settings ─────────────────────────────────────────────────────────
extern uint8_t  currentBrightness;    // 0–15
extern uint16_t scrollSpeed;          // ms per scroll frame

// ─── NTP / time ───────────────────────────────────────────────────────────────
extern bool     ntpSynced;            // true once first NTP sync is done

// ─── Clock mode ───────────────────────────────────────────────────────────────
extern bool     clockModeChangePending; // set by HTTP handler, cleared by display

// ─── Message ──────────────────────────────────────────────────────────────────
extern bool     messagePending;       // set by HTTP handler, cleared by display
extern char     messageText[];        // Latin-1 encoded, null-terminated
extern uint8_t  messageMode;          // MESSAGE_MODE_SCROLL / BLINK / STATIC
extern uint32_t messageDurationMs;    // duration for blink/static modes (ms)
extern int16_t  messageBrightness;    // per-message brightness override, 0–15; -1 = use currentBrightness
extern int32_t  messageScrollSpeedMs; // per-message scroll speed override, ms; -1 = use scrollSpeed
extern MessageHistoryEntry messageHistory[];  // ring buffer of recent messages
extern uint8_t  messageHistoryCount;  // number of valid entries (0..MESSAGE_HISTORY_SIZE)
extern uint8_t  messageHistoryHead;   // index of the oldest entry (ring buffer head)

// ─── Slot rotation ────────────────────────────────────────────────────────────
extern uint8_t  activeSlot;           // index of the currently active slot
extern bool     slotEnabled[];        // whether each slot is enabled
extern uint32_t slotIntervalMs[];     // how long each slot is displayed (ms)
extern uint16_t slotScheduleStartMin[]; // daily window start, minutes-of-day (0-1439); only used by slots 2/3
extern uint16_t slotScheduleEndMin[];   // daily window end, minutes-of-day (0-1439); start == end means "always"
extern uint8_t  slotScheduleDaysMask[]; // bit N = weekday N enabled (tm_wday: 0=Sunday..6=Saturday); only used by slots 2/3

// ─── Persistent configuration (loaded at boot, saved on change) ───────────────
extern char     cfgTimezone[];        // IANA timezone name (e.g. "America/Sao_Paulo")
extern char     cfgLocale[];           // "pt" or "en" — date names, number formatting, quote search language
extern char     cfgNtpServer[];       // NTP server hostname
extern char     cfgWifiSsid[];        // stored WiFi SSID (may be empty)
extern char     cfgWifiPass[];        // stored WiFi password
extern uint32_t cfgDateIntervalMs;    // how often date fires (ms); 0 = disabled
extern bool     cfgDateEnabled;       // true = show date periodically
extern char     cfgUiLanguage[];      // web panel UI language (e.g. "en", "pt") — independent of cfgLocale
extern uint8_t  cfgClockMode;         // CLOCK_MODE_HHMM or CLOCK_MODE_HHMMSS

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
