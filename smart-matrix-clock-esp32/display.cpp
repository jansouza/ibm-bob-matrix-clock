/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include "display.h"
#include "config.h"
#include "globals.h"
#include "locale_data.h"
#include "text_encoding.h"
#include "wifi_manager.h"
#include "date_font.h"
#include "data_fetcher.h"

#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

// ─── Internal objects ─────────────────────────────────────────────────────────

static MD_Parola _display(DISPLAY_HARDWARE, PIN_DATA, PIN_CLK, PIN_CS, NUM_MODULES);

// ─── Internal state ───────────────────────────────────────────────────────────

static uint32_t _lastBlink       = 0;
static bool     _colonVisible    = true;
static char     _lastTimeStr[8]  = "";   // last rendered string (avoid redundant writes)

// Scroll state
static bool     _scrolling        = false;  // true while a scroll animation is running
static char     _scrollBuf[SCROLL_BUF_LEN] = "";
static uint32_t _scrollDeadlineMs = 0;      // 0 = no limit; >0 = keep looping until this millis()

// AP config message — stored and re-scrolled until station connects
static char     _apMsgBuf[48]    = "";      // "Config 192.168.4.1"

// Date display timer
static uint32_t _lastDateShow    = 0;   // millis() when date was last triggered

// ── Static/blink alert state ──────────────────────────────────────────────────
static bool     _alertActive      = false;  // true while a static/blink alert is running
static uint32_t _alertStartMs     = 0;      // when the static/blink alert began
static uint32_t _alertBlinkLast   = 0;      // last blink toggle timestamp
static bool     _alertBlinkOn     = true;   // blink on/off state
static uint32_t _alertPhase1Ms    = 0;      // duration of the current alert's blink/static phase (ms)

// ── Blink+Scroll state ────────────────────────────────────────────────────────
// Phase 1: blink the first screen of text for ALERT_BLINK_SCROLL_PHASE1_MS
//          (capped to whatever remains of the cycle's total duration).
// Phase 2: scroll the remainder of alertMessage.
// The whole blink→scroll cycle then repeats from phase 1 until alertDurationMs
// (tracked from _alertCycleStartMs, not from each phase's own start) elapses.
static bool     _blinkScrollPhase2  = false;  // true once we transition from blink to scroll
static bool     _blinkScrollCycling = false;  // true while a repeating BLINK_SCROLL cycle is active
static uint32_t _alertCycleStartMs  = 0;      // millis() when the overall BLINK_SCROLL cycle began

// ── Fixed date display state ──────────────────────────────────────────────────
// Shows "DAY DDMM" (or "DAY MMDD") all at once using the small 3×5 font.
// Duration is the full cfgDateIntervalMs; then we return to clock.
#define DATE_SHOW_MS  5000   // how long the date stays on screen (ms)

static bool     _dateActive       = false;
static uint32_t _dateMsStart      = 0;
static char     _dateLine[12]     = "";  // e.g. "SEG 1407" or "MON 0714"

// ── Slot rotation state ───────────────────────────────────────────────────────
static uint32_t _slotStartMs      = 0;   // when the current non-clock slot began
static bool     _slotActive       = false; // true while a non-clock slot is running
static int8_t   _forceSlotIndex   = -1;   // set by displayForceSlot(); consumed by displayTick()

// ─── Internal helpers ─────────────────────────────────────────────────────────

// Resolve language index from cfgLanguage string.
static uint8_t _langIndex() {
    return (cfgLanguage[0] == 'p' || cfgLanguage[0] == 'P') ? LANG_PT : LANG_EN;
}

// Start a non-blocking scroll of the given Latin-1 string at the given speed.
// durationMs = 0  → scroll plays until the text has fully scrolled off (original behaviour).
// durationMs > 0  → scroll is cut off after that many milliseconds.
static void _startScrollAt(const char* latin1Text, uint16_t speed, uint32_t durationMs = 0) {
    strncpy(_scrollBuf, latin1Text, SCROLL_BUF_LEN - 1);
    _scrollBuf[SCROLL_BUF_LEN - 1] = '\0';

    _display.displayClear();
    _display.displayText(_scrollBuf, PA_LEFT, speed, 0,
                         PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    _scrolling        = true;
    _scrollDeadlineMs = (durationMs > 0) ? (millis() + durationMs) : 0;
    _lastTimeStr[0]   = '\0';   // force time redraw after scroll ends
}

// Start a non-blocking scroll at the configured scrollSpeed.
static void _startScroll(const char* latin1Text, uint32_t durationMs = 0) {
    uint16_t speed = (alertScrollSpeedMs >= 0) ? (uint16_t)alertScrollSpeedMs : scrollSpeed;
    _startScrollAt(latin1Text, speed, durationMs);
}

// Apply the alert's brightness override (if any) on top of currentBrightness.
static void _applyAlertBrightness() {
    if (alertBrightness >= 0) _display.setIntensity((uint8_t)alertBrightness);
}

// Restore the globally configured brightness after an alert ends.
static void _restoreBrightness() {
    if (alertBrightness >= 0) _display.setIntensity(currentBrightness);
}

// Number of columns that fit on the physical display (8 columns per module).
#define DISPLAY_WIDTH_PX  (NUM_MODULES * 8)

// Count how many leading characters of a Latin-1 string fit within the
// display width, given the active font's per-character width plus the
// 1px inter-character spacing MD_MAX72XX/Parola always insert.
static size_t _charsFittingOnScreen(const char* text) {
    uint8_t buf[8];
    int16_t usedPx = 0;
    size_t  count  = 0;

    for (const uint8_t* p = (const uint8_t*)text; *p; p++) {
        uint8_t w = _display.getGraphicObject()->getChar(*p, sizeof(buf), buf);
        int16_t nextPx = usedPx + w + (count > 0 ? 1 : 0);
        if (nextPx > DISPLAY_WIDTH_PX) break;
        usedPx = nextPx;
        count++;
    }
    return count;
}

// Print a static string centred on the display.
static void _printCentered(const char* text) {
    _display.displayClear();
    _display.setTextAlignment(PA_CENTER);
    _display.print(text);
    _lastTimeStr[0] = '\0';
}

// Begin a static or blink alert.
// isCycleStart: true when this call begins a brand-new BLINK_SCROLL cycle
// (as opposed to re-entering phase 1 after phase 2's scroll finished).
static void _startTimedAlert(bool isCycleStart = true) {
    _alertActive       = true;
    _alertStartMs      = millis();
    _alertBlinkLast    = millis();
    _alertBlinkOn      = true;
    _blinkScrollPhase2 = false;

    if (alertMode == ALERT_MODE_BLINK_SCROLL) {
        if (isCycleStart) _alertCycleStartMs = _alertStartMs;
        _blinkScrollCycling = true;
        // Phase 1 gets a fixed slice, capped to whatever remains of the
        // overall cycle budget (alertDurationMs, measured from cycle start).
        uint32_t cycleElapsed = _alertStartMs - _alertCycleStartMs;
        uint32_t cycleRemaining = (alertDurationMs > cycleElapsed) ? (alertDurationMs - cycleElapsed) : 0;
        _alertPhase1Ms = (cycleRemaining < ALERT_BLINK_SCROLL_PHASE1_MS)
                             ? cycleRemaining : ALERT_BLINK_SCROLL_PHASE1_MS;
    } else {
        _blinkScrollCycling = false;
        _alertPhase1Ms = alertDurationMs;
    }

    _applyAlertBrightness();
    _printCentered(alertMessage);
}

// Show date in small font: "DAY DD MM" or "DAY MM DD" — printed with Parola
// (so column ordering stays exactly as it works for every other screen using
// this display), then touched up in place:
//   - if "DAY DD MM" would leave less than DATE_RIGHT_MARGIN_PX free on the
//     right (it commonly does — this font's glyph widths make the full
//     string 31-33px wide against a 32-column display, depending on which
//     digits appear), the space between DD and MM is dropped to shrink it
//   - the string is then repositioned (not just left as PA_CENTER placed it)
//     so it keeps that right margin, instead of trusting PA_CENTER's own
//     centring math, which has zero slack at these widths and pins the text
//     against the right edge
//   - the day-of-week glyphs are shifted 1 pixel row lower, so a decorative
//     rule can be drawn on row 0 directly above them (day's columns only)
//
// IMPORTANT — FC16_HW column direction:
//   MD_MAX72XX with FC16_HW reverses column indices so that raw column 0 is
//   the rightmost physical pixel and raw column 31 is the leftmost.
//   Parola's PA_CENTER formula (and the repositioning shift below) operates in
//   this same raw space.  After repositioning:
//     rightEdge = raw column of the visual-LEFT edge  (day name start)
//     leftEdge  = raw column of the visual-RIGHT edge (date numbers end)
//   The day-of-week decoration loop must therefore walk from rightEdge DOWN
//   by dayWidth columns, not from leftEdge up.
#define DATE_RIGHT_MARGIN_PX  1

static void _startDateDisplay() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo) || (timeinfo.tm_year + 1900) < 2020) return;

    uint8_t lang = _langIndex();
    const char* day = localeDayName(lang, (uint8_t)timeinfo.tm_wday);
    char dateNums[8];

    if (lang == LANG_PT) {
        snprintf(dateNums, sizeof(dateNums), "%02d %02d", timeinfo.tm_mday, timeinfo.tm_mon + 1);
    } else {
        snprintf(dateNums, sizeof(dateNums), "%02d %02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    }
    snprintf(_dateLine, sizeof(_dateLine), "%s %s", day, dateNums);

    _display.displayClear();
    _display.setFont(_dateSmallFont);
    _display.setTextAlignment(PA_CENTER);

    const uint16_t displayWidth = NUM_MODULES * 8;
    uint16_t textWidth = _display.getTextColumns(_dateLine);
    if (textWidth > displayWidth - DATE_RIGHT_MARGIN_PX) {
        // Drop the DD-MM space to buy back 1px.
        if (lang == LANG_PT) {
            snprintf(dateNums, sizeof(dateNums), "%02d%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1);
        } else {
            snprintf(dateNums, sizeof(dateNums), "%02d%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
        }
        snprintf(_dateLine, sizeof(_dateLine), "%s %s", day, dateNums);
        textWidth = _display.getTextColumns(_dateLine);
    }

    _display.print(_dateLine);

    MD_MAX72XX* graphics = _display.getGraphicObject();

    // Where PA_CENTER actually placed the text (its own centring formula,
    // from MD_PZone::calcTextLimits — reproduced here to locate the pixels).
    uint16_t parolaRight = (displayWidth + textWidth - 1) / 2;
    uint16_t parolaLeft  = parolaRight - textWidth + 1;

    // Where we want it: true centre, clamped to keep DATE_RIGHT_MARGIN_PX
    // clear on the right (this is the margin users reported text touching).
    int16_t desiredLeft = (int16_t)((displayWidth - textWidth) / 2);
    if (desiredLeft + (int16_t)textWidth > displayWidth - DATE_RIGHT_MARGIN_PX)
        desiredLeft = displayWidth - DATE_RIGHT_MARGIN_PX - (int16_t)textWidth;
    if (desiredLeft < 0) desiredLeft = 0;

    int16_t shift = desiredLeft - (int16_t)parolaLeft;
    if (shift < 0) {
        // Move left: copy ascending so the (lower) destination index is
        // always processed before its source is overwritten.
        for (uint16_t c = parolaLeft; c <= parolaRight; c++)
            graphics->setColumn((uint16_t)((int16_t)c + shift), graphics->getColumn(c));
        for (uint16_t c = (uint16_t)((int16_t)parolaRight + shift + 1); c <= parolaRight; c++)
            graphics->setColumn(c, 0);
    } else if (shift > 0) {
        // Move right: copy descending for the same reason, reversed.
        for (int16_t c = parolaRight; c >= (int16_t)parolaLeft; c--)
            graphics->setColumn((uint16_t)(c + shift), graphics->getColumn((uint16_t)c));
        for (uint16_t c = parolaLeft; c < (uint16_t)(parolaLeft + shift); c++)
            graphics->setColumn(c, 0);
    }

    uint16_t leftEdge  = (uint16_t)desiredLeft;
    uint16_t rightEdge = leftEdge + textWidth - 1;

    // Shift the day-of-week's own columns down by 1 row and draw the rule on
    // row 0 above them only.
    // For FC16_HW, raw indices are reversed: the day name (visual left) sits at
    // the HIGHEST raw column indices (rightEdge downward), not at leftEdge.
    uint16_t dayWidth    = _display.getTextColumns(day);
    uint16_t dayRawRight = rightEdge;
    uint16_t dayRawLeft  = (dayWidth <= rightEdge + 1)
                               ? (rightEdge - dayWidth + 1) : 0;
    for (uint16_t c = dayRawLeft; c <= dayRawRight; c++) {
        uint8_t colByte = graphics->getColumn(c);
        graphics->setColumn(c, (uint8_t)(colByte << 1));
        graphics->setPoint(0, c, true);
    }

    _dateActive   = true;
    _dateMsStart  = millis();
    _lastTimeStr[0] = '\0';
}

// ─── Slot rotation ────────────────────────────────────────────────────────────

// Build the weather display string into dst[dstLen].
// Format: "22°C Cloudy Min18 Max27" (or with * prefix if stale).
// Returns false if the cache is not valid (slot should be skipped).
static bool _buildWeatherString(char* dst, size_t dstLen) {
    if (!weatherCache.valid) return false;

    bool isFahrenheit = (cfgTempUnit[0] == 'F' || cfgTempUnit[0] == 'f');
    char unitCh = isFahrenheit ? 'F' : 'C';

    // Round temperatures to nearest integer for compact display
    int temp = (int)(weatherCache.temp + (weatherCache.temp >= 0 ? 0.5f : -0.5f));
    int tmin = (int)(weatherCache.minTemp + (weatherCache.minTemp >= 0 ? 0.5f : -0.5f));
    int tmax = (int)(weatherCache.maxTemp + (weatherCache.maxTemp >= 0 ? 0.5f : -0.5f));

    // The degree sign in Latin-1 is 0xB0 (°)
    snprintf(dst, dstLen, "%s%d\xB0%c %s Min%d Max%d",
             weatherCache.stale ? "*" : "",
             temp, unitCh,
             weatherCache.condition,
             tmin, tmax);
    return true;
}

// Start a slot scroll immediately (shared by timer-driven and forced paths).
// Returns true if a scroll was started.
static bool _startSlotScroll(uint8_t slot) {
    if (slot == 2) {
        char buf[SCROLL_BUF_LEN];
        if (_buildWeatherString(buf, sizeof(buf))) {
            _slotStartMs = millis();
            _slotActive  = true;
            activeSlot   = 2;
            _startScrollAt(buf, scrollSpeed);
            return true;
        }
    }
    return false;
}

// Check slot rotation timer and trigger next slot when due.
// Must be called from displayTick() only when the display is idle (not
// scrolling, not alerting, not showing date).
static void _slotRotationTick() {
    uint32_t now = millis();

    // If a slot scroll was active and has now finished, clear the active flag.
    if (_slotActive) {
        if (_scrolling) return;   // still scrolling — nothing to do yet
        _slotActive = false;
        activeSlot  = 0;          // return to clock
        // Reset timer from when this slot ended, not when it started.
        _slotStartMs = now;
        return;   // give clock one tick to redraw before next check
    }

    // Forced preview requested by displayForceSlot() (from HTTP handler).
    if (_forceSlotIndex >= 0) {
        uint8_t s = (uint8_t)_forceSlotIndex;
        _forceSlotIndex = -1;
        _startSlotScroll(s);   // ignore return — if no cache, just skip
        return;
    }

    // Timer-driven rotation: slot 2 = Weather (slot 3 = Quotes, Phase 5).
    if (slotEnabled[2] && weatherCache.valid) {
        if (now - _slotStartMs >= slotIntervalMs[2]) {
            _startSlotScroll(2);
        }
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

void displayBegin() {
    _display.begin();
    _display.setIntensity(currentBrightness);
    _display.displayClear();
    _display.setTextAlignment(PA_CENTER);
    _display.print("--:--");
    _lastDateShow = millis();   // don't show date immediately on boot
    _slotStartMs  = millis();   // don't show weather slot immediately on boot
}

void displayTick() {
    uint32_t now = millis();

    // ── Advance scroll animation ───────────────────────────────────────────────
    if (_scrolling) {
        if (_display.displayAnimate()) {
            // One pass finished — check whether to loop or stop
            bool keepLooping = (_scrollDeadlineMs > 0 && now < _scrollDeadlineMs);

            if (keepLooping) {
                // Restart the same text for another pass
                _display.displayReset();
                return;
            }

            // Done — stop scrolling
            _scrolling = false;
            _display.displayClear();
            _display.setTextAlignment(PA_CENTER);

            // BLINK_SCROLL: if the overall cycle still has time left, go back
            // to phase 1 (blink) instead of returning to the clock.
            if (_blinkScrollCycling) {
                uint32_t cycleElapsed = now - _alertCycleStartMs;
                if (cycleElapsed < alertDurationMs) {
                    _startTimedAlert(false);
                    return;
                }
                _blinkScrollCycling = false;
            }

            _restoreBrightness();
            alertBrightness    = -1;
            alertScrollSpeedMs = -1;

            // In AP mode, immediately re-scroll the config message
            if (isApMode() && _apMsgBuf[0] != '\0') {
                _startScrollAt(_apMsgBuf, AP_SCROLL_SPEED_MS);
                return;
            }
        } else {
            return;   // still scrolling — nothing else to do this tick
        }
    }

    // ── Static / blink alert ──────────────────────────────────────────────────
    if (_alertActive) {
        uint32_t elapsed = now - _alertStartMs;
        if (elapsed >= _alertPhase1Ms) {
            // Blink phase done — if BLINK_SCROLL, transition to scroll phase.
            // Only scroll the part of the message that wasn't already shown
            // (blinking) during phase 1.
            if (alertMode == ALERT_MODE_BLINK_SCROLL && !_blinkScrollPhase2) {
                _blinkScrollPhase2 = true;
                size_t shown = _charsFittingOnScreen(alertMessage);
                if (alertMessage[shown] != '\0') {
                    _alertActive = false;
                    // Remainder of the overall cycle budget goes to phase 2.
                    uint32_t cycleElapsed = now - _alertCycleStartMs;
                    uint32_t remaining = (alertDurationMs > cycleElapsed) ? (alertDurationMs - cycleElapsed) : 0;
                    _startScroll(alertMessage + shown, remaining);
                    return;
                }
                // Whole message already fits on screen — nothing to scroll.
                // If the cycle still has time left, just restart phase 1 (re-blink)
                // instead of ending the alert early.
                uint32_t cycleElapsed = now - _alertCycleStartMs;
                if (cycleElapsed < alertDurationMs) {
                    _startTimedAlert(false);
                    return;
                }
                _blinkScrollCycling = false;
            }
            // Alert duration expired — return to clock
            _alertActive = false;
            _display.displayClear();
            _display.setTextAlignment(PA_CENTER);
            _restoreBrightness();
            alertBrightness    = -1;
            alertScrollSpeedMs = -1;
            _lastTimeStr[0] = '\0';
        } else if (alertMode == ALERT_MODE_BLINK || alertMode == ALERT_MODE_BLINK_SCROLL) {
            if (now - _alertBlinkLast >= ALERT_BLINK_PERIOD_MS) {
                _alertBlinkLast = now;
                _alertBlinkOn   = !_alertBlinkOn;
                _display.displayClear();
                _display.setTextAlignment(PA_CENTER);
                if (_alertBlinkOn) _display.print(alertMessage);
            }
        }
        // ALERT_MODE_STATIC: just leave the text on screen until duration expires
        return;
    }

    // ── Fixed date display ────────────────────────────────────────────────────
    if (_dateActive) {
        if (now - _dateMsStart >= DATE_SHOW_MS) {
            // Date done — restore default font and return to clock
            _dateActive = false;
            _display.displayClear();
            _display.setFont(nullptr);   // nullptr = revert to system font
            _display.setTextAlignment(PA_CENTER);
            _lastTimeStr[0] = '\0';
        }
        return;   // don't update clock while date is showing
    }

    // ── Check for pending alert (one-shot, highest priority after scroll) ──────
    if (alertPending) {
        alertPending = false;
        _blinkScrollPhase2 = false;
        if (alertMode == ALERT_MODE_SCROLL) {
            _applyAlertBrightness();
            _startScroll(alertMessage, alertDurationMs);
        } else {
            // ALERT_MODE_BLINK, ALERT_MODE_STATIC, ALERT_MODE_BLINK_SCROLL all start timed
            _startTimedAlert();
        }
        return;
    }

    // ── Periodic date display ──────────────────────────────────────────────────
    if (cfgDateEnabled && ntpSynced && cfgDateIntervalMs > 0) {
        if (now - _lastDateShow >= cfgDateIntervalMs) {
            _lastDateShow = now;
            _startDateDisplay();
            return;
        }
    }

    // ── Slot rotation (weather slot; quotes in Phase 5) ───────────────────────
    _slotRotationTick();
    if (_scrolling) return;   // rotation just started a scroll — done this tick

    // ── Colon blink every BLINK_INTERVAL_MS ───────────────────────────────────
    if (now - _lastBlink >= BLINK_INTERVAL_MS) {
        _lastBlink    = now;
        _colonVisible = !_colonVisible;

        char buf[8];

        if (!ntpSynced) {
            snprintf(buf, sizeof(buf), "%s", _colonVisible ? "--:--" : "-- --");
        } else {
            struct tm timeinfo;
            if (!getLocalTime(&timeinfo)) {
                snprintf(buf, sizeof(buf), "--:--");
            } else {
                snprintf(buf, sizeof(buf), _colonVisible ? "%02d:%02d" : "%02d %02d",
                         timeinfo.tm_hour, timeinfo.tm_min);
            }
        }

        // Only write to display when the string actually changes
        if (strcmp(buf, _lastTimeStr) != 0) {
            strncpy(_lastTimeStr, buf, sizeof(_lastTimeStr) - 1);
            _lastTimeStr[sizeof(_lastTimeStr) - 1] = '\0';
            _display.setFont(nullptr);   // ensure system font is active
            _display.setTextAlignment(PA_CENTER);
            _display.print(buf);
        }
    }
}

void setBrightness(uint8_t level) {
    currentBrightness = level;
    _display.setIntensity(level);
}

void setScrollSpeed(uint16_t ms) {
    scrollSpeed = ms;
    _display.setSpeed(ms);
}

void displayScrollText(const char* latin1Text) {
    _startScroll(latin1Text);
}

void displaySetApMessage(const char* latin1Text) {
    strncpy(_apMsgBuf, latin1Text, sizeof(_apMsgBuf) - 1);
    _apMsgBuf[sizeof(_apMsgBuf) - 1] = '\0';
    _startScrollAt(_apMsgBuf, AP_SCROLL_SPEED_MS);
}

void displayForceSlot(uint8_t slotIndex) {
    // Safe to call from an HTTP handler — only sets a flag.
    // displayTick() / _slotRotationTick() consumes it on the next loop iteration.
    _forceSlotIndex = (int8_t)slotIndex;
}
