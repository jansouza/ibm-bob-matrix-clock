#pragma once

/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */
#include <stdint.h>

// Initialise NTP and start background time sync.
// server       — NTP server hostname (e.g. "pool.ntp.org")
// timezone_posix — POSIX TZ string (e.g. "BRT3" or "BRST3BRDT2,M10.3.0,M2.3.0")
// Must be called after WiFi is connected.
void ntpBegin(const char* server, const char* timezone_posix);

// Called every loop() iteration.
// Sets ntpSynced = true once the system clock is valid.
// Handles periodic re-synchronisation every NTP_RESYNC_MS milliseconds.
void ntpTick();
