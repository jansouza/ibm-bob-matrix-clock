#include "ntp.h"
#include "config.h"
#include "globals.h"

#include <Arduino.h>
#include <time.h>

// ─── Internal state ───────────────────────────────────────────────────────────

static uint32_t _lastCheck  = 0;
static uint32_t _lastResync = 0;
static bool     _started    = false;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static bool _clockIsValid() {
    struct tm timeinfo;
    // getLocalTime returns false / epoch-like time when not synced
    if (!getLocalTime(&timeinfo)) return false;
    // ESP32 epoch after configTime: year >= 2020 means sync succeeded
    return (timeinfo.tm_year + 1900) >= 2020;
}

// ─── Public API ───────────────────────────────────────────────────────────────

void ntpBegin(const char* server, const char* timezone_posix) {
    if (!server || server[0] == '\0') server = cfgNtpServer;
    configTime(0, 0, server);   // UTC offset = 0; POSIX TZ handles timezone
    // Apply timezone only if a POSIX string was explicitly passed.
    // In Phase 2+, timezone is already applied via applyTimezone() before ntpBegin().
    if (timezone_posix != nullptr) {
        setenv("TZ", timezone_posix, 1);
        tzset();
    }
    _started    = true;
    _lastResync = millis();
}

void ntpTick() {
    if (!_started) return;

    uint32_t now = millis();

    // ── Poll for initial sync ──────────────────────────────────────────────────
    if (!ntpSynced && (now - _lastCheck >= NTP_CHECK_INTERVAL_MS)) {
        _lastCheck = now;
        if (_clockIsValid()) {
            ntpSynced = true;
        }
    }

    // ── Periodic re-sync ──────────────────────────────────────────────────────
    if (now - _lastResync >= NTP_RESYNC_MS) {
        _lastResync = now;
        // configTime triggers a new SNTP request; ntpSynced stays true
        // because the clock keeps its last-known value while re-syncing
        configTime(0, 0, cfgNtpServer);
    }
}
