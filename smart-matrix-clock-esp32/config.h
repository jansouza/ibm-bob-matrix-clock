#pragma once

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

#define DEFAULT_BRIGHTNESS       4    // 0–15
#define DEFAULT_SCROLL_SPEED_MS  50   // ms per scroll frame (10–200)

// ─── Text buffer ──────────────────────────────────────────────────────────────
#define MAX_ALERT_LEN  128   // max chars for alert message (incl. null)
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
#define NTP_TIMEZONE_DEFAULT    "America/Sao_Paulo"
#define NTP_TIMEZONE_MAX        48
#define NTP_RESYNC_MS           3600000UL  // re-sync every 1 hour
#define NTP_CHECK_INTERVAL_MS   500        // how often ntpTick() polls for sync

// ─── Blink ────────────────────────────────────────────────────────────────────
#define BLINK_INTERVAL_MS      500   // colon blink period (ms)
#define AP_SCROLL_SPEED_MS     80    // slower scroll speed for AP config message (ms/frame)

// ─── Locale ───────────────────────────────────────────────────────────────────
#define LANG_CODE_MAX   4    // "pt\0" or "en\0"
#define LANG_DEFAULT    "pt"

// ─── Date display ─────────────────────────────────────────────────────────────
#define DATE_INTERVAL_DEFAULT_MS  30000UL   // show date every 30 s
#define DATE_INTERVAL_MIN_MS       5000UL   // minimum 5 s
#define DATE_INTERVAL_MAX_MS     300000UL   // maximum 5 min

// ─── Alert display mode ───────────────────────────────────────────────────────
#define ALERT_MODE_SCROLL       0   // scroll text left (original behaviour)
#define ALERT_MODE_BLINK        1   // blink text on/off for alertDurationMs
#define ALERT_MODE_STATIC       2   // show text static for alertDurationMs
#define ALERT_MODE_BLINK_SCROLL 3   // blink first screen for alertDurationMs, then scroll remainder

#define ALERT_DURATION_DEFAULT_MS  5000UL   // default static/blink duration (ms)
#define ALERT_BLINK_PERIOD_MS       500     // blink toggle period (ms)

// ─── API authentication ───────────────────────────────────────────────────────
#define API_KEY_LEN   33    // 32 hex chars + null terminator
#define API_KEY_HEADER "X-API-Key"

// ─── NVS namespace and keys ───────────────────────────────────────────────────
#define NVS_NAMESPACE       "clk"

#define NVS_KEY_BRIGHTNESS  "brightness"
#define NVS_KEY_SCROLL_SPD  "scroll_spd"
#define NVS_KEY_TIMEZONE    "timezone"
#define NVS_KEY_LANGUAGE    "language"
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
#define NVS_KEY_ALERT_MODE  "alert_mode"
#define NVS_KEY_ALERT_DUR   "alert_dur"
#define NVS_KEY_API_KEY     "api_key"
#define NVS_KEY_API_AUTH    "api_auth"

// ─── Restart ──────────────────────────────────────────────────────────────────
#define RESTART_DELAY_MS  1500   // default deferred restart delay
