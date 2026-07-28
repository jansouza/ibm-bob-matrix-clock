#pragma once

/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

// ─── Firmware version ─────────────────────────────────────────────────────────
#define FIRMWARE_VERSION "1.0.0"

// ─── Hardware — SPI pins (VSPI defaults) ──────────────────────────────────────
#define PIN_CLK     18   // VSPI CLK
#define PIN_DATA    23   // VSPI MOSI
#define PIN_CS       5   // VSPI CS

// ─── Hardware — Boot / factory-reset button ───────────────────────────────────
#define PIN_BOOT     0   // GPIO 0 — BOOT button (active LOW)
#define BOOT_HOLD_MS 3000  // hold duration to trigger factory reset (ms)

// ─── Display ──────────────────────────────────────────────────────────────────
#define NUM_MODULES         4    // number of chained MAX7219 8×8 modules
#define DISPLAY_HARDWARE    MD_MAX72XX::FC16_HW

#define DEFAULT_BRIGHTNESS       2    // 0–15
#define DEFAULT_SCROLL_SPEED_MS  70   // ms per scroll frame (10–200)

// ─── Text buffer ──────────────────────────────────────────────────────────────
#define MAX_MESSAGE_LEN  128   // max chars for message (incl. null)
#define SCROLL_BUF_LEN 256   // internal scroll working buffer

// ─── WiFi ─────────────────────────────────────────────────────────────────────
#define WIFI_TIMEOUT_MS       10000   // 10 s station-connect timeout
#define WIFI_RECONNECT_MS     30000   // how often wifiTick() retries if disconnected
#define WIFI_AP_SSID          "SmartMatrixClock-Setup"
#define WIFI_AP_PASSWORD      ""     // open AP
#define WIFI_AP_IP            "192.168.4.1"

// Max lengths for stored credentials (incl. null terminator)
#define WIFI_SSID_MAX     33
#define WIFI_PASS_MAX     65

// ─── NTP ──────────────────────────────────────────────────────────────────────
#define NTP_SERVER_DEFAULT      "pool.ntp.org"
#define NTP_SERVER_MAX          64
#define NTP_TIMEZONE_DEFAULT    "UTC"
#define NTP_TIMEZONE_MAX        48
#define NTP_RESYNC_MS           3600000UL  // re-sync every 1 hour
#define NTP_CHECK_INTERVAL_MS   500        // how often ntpTick() polls for sync

// ─── Blink ────────────────────────────────────────────────────────────────────
#define BLINK_INTERVAL_MS      500   // colon blink period (ms)
#define AP_SCROLL_SPEED_MS     80    // slower scroll speed for AP config message (ms/frame)

// ─── Content locale ─────────────────────────────────────────────────────────────
// Governs everything content-related: date weekday/month names, the quote
// search language sent to Yahoo Finance, and number formatting (thousands/
// decimal separators). See cfgLocale in globals.h.
#define LOCALE_CODE_MAX   4    // "pt\0" or "en\0"
#define LOCALE_DEFAULT    "en"

// ─── Web UI language ───────────────────────────────────────────────────────────
// Separate from LOCALE_DEFAULT/cfgLocale above, which only controls the
// on-device clock/date/number/quote-search locale. This controls the web
// panel's own interface language and is independent of the display's locale.
// The list of accepted codes lives in persistence.cpp (isUiLanguageValid()) —
// add a new entry there (and a matching I18N dictionary in web_page.cpp)
// to support another language, no other branching logic needed.
#define UI_LANG_CODE_MAX  4      // "en\0" or "pt\0"
#define UI_LANG_DEFAULT   "en"

// ─── Date display ─────────────────────────────────────────────────────────────
#define DATE_INTERVAL_DEFAULT_MS  30000UL   // show date every 30 s
#define DATE_INTERVAL_MIN_MS       5000UL   // minimum 5 s
#define DATE_INTERVAL_MAX_MS     300000UL   // maximum 5 min

// ─── Clock display mode ────────────────────────────────────────────────────────
#define CLOCK_MODE_HHMM           0   // HH:MM  (default, colon blinks)
#define CLOCK_MODE_HHMMSS         1   // HH:MM:SS  (seconds visible, updates every second)

#define CLOCK_MODE_DEFAULT        CLOCK_MODE_HHMM
#define NVS_KEY_CLOCK_MODE        "clock_mode"

// ─── Message display mode ──────────────────────────────────────────────────────
#define MESSAGE_MODE_SCROLL       0   // scroll text left (original behaviour)
#define MESSAGE_MODE_BLINK        1   // blink text on/off for messageDurationMs
#define MESSAGE_MODE_STATIC       2   // show text static for messageDurationMs
#define MESSAGE_MODE_BLINK_SCROLL 3   // blink first screen, then scroll remainder, repeating the cycle; messageDurationMs is the TOTAL for the whole repeating cycle

#define MESSAGE_DURATION_DEFAULT_MS  5000UL   // default static/blink duration (ms)
#define MESSAGE_HISTORY_SIZE         20       // ring buffer capacity (number of entries)
#define MESSAGE_BLINK_PERIOD_MS       500     // blink toggle period (ms)
#define MESSAGE_BLINK_SCROLL_PHASE1_MS 5000UL // fixed phase-1 (blink) duration for MESSAGE_MODE_BLINK_SCROLL (ms)

// ─── NVS namespace and keys ───────────────────────────────────────────────────
#define NVS_NAMESPACE       "clk"

#define NVS_KEY_BRIGHTNESS  "brightness"
#define NVS_KEY_SCROLL_SPD  "scroll_spd"
#define NVS_KEY_TIMEZONE    "timezone"
#define NVS_KEY_LOCALE      "locale"
#define NVS_KEY_NTP_SERVER  "ntp_server"
#define NVS_KEY_WIFI_SSID   "wifi_ssid"
#define NVS_KEY_WIFI_PASS   "wifi_pass"
#define NVS_KEY_SLOT0_EN    "slot0_en"
#define NVS_KEY_SLOT1_EN    "slot1_en"
#define NVS_KEY_SLOT2_EN    "slot2_en"
#define NVS_KEY_SLOT3_EN    "slot3_en"
#define NVS_KEY_SLOT2_MS    "slot2_ms"
#define NVS_KEY_SLOT3_MS    "slot3_ms"
#define NVS_KEY_DATE_INT_MS "date_int_ms"
#define NVS_KEY_DATE_EN     "date_en"
#define NVS_KEY_MESSAGE_MODE "msg_mode"
#define NVS_KEY_MESSAGE_DUR  "msg_dur"
#define NVS_KEY_UI_LANGUAGE "ui_lang"

// ─── Restart ──────────────────────────────────────────────────────────────────
#define RESTART_DELAY_MS  1500   // default deferred restart delay

// ─── Weather slot (Phase 4) ───────────────────────────────────────────────────
#define WEATHER_UPDATE_DEFAULT_MS   600000UL  // fetch interval: 10 minutes
#define WEATHER_UPDATE_MIN_MS        60000UL  // minimum 1 minute
#define WEATHER_UPDATE_MAX_MS      3600000UL  // maximum 1 hour
#define WEATHER_DISPLAY_DEFAULT_MS   30000UL  // display slot duration: 30 s
#define WEATHER_DISPLAY_MIN_MS        5000UL
#define WEATHER_DISPLAY_MAX_MS      300000UL
#define WEATHER_LAT_DEFAULT           -23.55f  // São Paulo
#define WEATHER_LON_DEFAULT           -46.63f
#define WEATHER_TEMP_UNIT_DEFAULT     "C"     // "C" or "F"
#define WEATHER_TEMP_UNIT_MAX          3      // "C\0" or "F\0"
#define WEATHER_CONDITION_MAX         24      // max chars for condition string

// NVS keys — weather
#define NVS_KEY_WEATHER_EN    "weather_en"
#define NVS_KEY_WEATHER_UPMS  "weather_upms"
#define NVS_KEY_WEATHER_LAT   "weather_lat"
#define NVS_KEY_WEATHER_LON   "weather_lon"
#define NVS_KEY_TEMP_UNIT     "temp_unit"

// ─── Quotes slot (Phase 5) ─────────────────────────────────────────────────────
#define QUOTES_UPDATE_DEFAULT_MS    600000UL  // fetch interval: 10 minutes
#define QUOTES_UPDATE_MIN_MS         60000UL  // minimum 1 minute
#define QUOTES_UPDATE_MAX_MS       3600000UL  // maximum 1 hour
#define QUOTES_DISPLAY_DEFAULT_MS    30000UL  // display slot duration: 30 s
#define QUOTES_DISPLAY_MIN_MS         5000UL
#define QUOTES_DISPLAY_MAX_MS       300000UL

#define QUOTES_MAX_TICKERS       8    // maximum number of symbols
#define QUOTES_SYMBOL_MAX        12   // max chars per symbol (incl. null)
#define QUOTES_TICKERS_MAX       120  // max length of the comma-separated tickers string (incl. null)

// Default schedule: business hours, Monday-Friday (markets are closed nights/weekends).
// Minutes-of-day; see "Slot scheduling by time of day" below for the encoding.
#define QUOTES_SCHED_START_DEFAULT_MIN  480    // 08:00
#define QUOTES_SCHED_END_DEFAULT_MIN   1080    // 18:00
#define QUOTES_SCHED_DAYS_DEFAULT  ((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5))  // Mon-Fri

// NVS keys — quotes
#define NVS_KEY_QUOTES_EN      "quotes_en"
#define NVS_KEY_QUOTES_UPMS    "quotes_upms"
#define NVS_KEY_QUOTES_TICKERS "quotes_tick"

// ─── Slot scheduling by time of day (Sub-Task 6e) ─────────────────────────────
// Each rotating slot (weather = 2, quotes = 3) may be restricted to a daily
// time window, expressed as minutes-of-day (0-1439). start == end means "no
// restriction" (slot follows only its enabled flag, as before). start > end
// wraps past midnight (e.g. 22:00-06:00 covers the overnight window).
#define SLOT_SCHEDULE_MIN_MINUTE   0
#define SLOT_SCHEDULE_MAX_MINUTE   1439

// Weekday mask: bit N set = day N enabled, where N follows struct tm's
// tm_wday convention (0 = Sunday .. 6 = Saturday). All-bits-set = every day.
#define SLOT_SCHEDULE_ALL_DAYS     0x7F
#define SLOT_SCHEDULE_DAYS_MAX     0x7F   // mask upper bound (7 bits)

// NVS keys — slot schedule
#define NVS_KEY_SLOT2_SCHED_START "s2_sch_st"
#define NVS_KEY_SLOT2_SCHED_END   "s2_sch_en"
#define NVS_KEY_SLOT2_SCHED_DAYS  "s2_sch_dy"
#define NVS_KEY_SLOT3_SCHED_START "s3_sch_st"
#define NVS_KEY_SLOT3_SCHED_END   "s3_sch_en"
#define NVS_KEY_SLOT3_SCHED_DAYS  "s3_sch_dy"

// ─── Auto brightness by time of day (Sub-Task 6b) ─────────────────────────────
// When enabled, the display automatically dims during the configured night
// window (start..end in minutes-of-day). start == end means disabled.
// start > end wraps past midnight (e.g. 23:00-07:00).
#define DEFAULT_NIGHT_BRIGHTNESS_ENABLED  false
#define DEFAULT_NIGHT_BRIGHTNESS_LEVEL    0      // fully off at night (0–15)
#define DEFAULT_NIGHT_START_MIN           1380   // 23:00 in minutes-of-day
#define DEFAULT_NIGHT_END_MIN             420    // 07:00 in minutes-of-day

#define NVS_KEY_NIGHT_BRI_EN   "night_bri_en"   // 12 chars
#define NVS_KEY_NIGHT_BRI_LVL  "night_bri_lvl"  // 13 chars
#define NVS_KEY_NIGHT_START    "night_start"     // 11 chars
#define NVS_KEY_NIGHT_END      "night_end"       //  9 chars
