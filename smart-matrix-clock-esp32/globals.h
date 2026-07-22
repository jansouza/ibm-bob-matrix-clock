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
