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
#include <math.h>

// ─── Internal objects ─────────────────────────────────────────────────────────

static MD_Parola _display(DISPLAY_HARDWARE, PIN_DATA, PIN_CLK, PIN_CS, NUM_MODULES);

// ─── Internal state ───────────────────────────────────────────────────────────

static uint32_t _lastBlink       = 0;
static bool     _colonVisible    = true;
static char     _lastTimeStr[10] = "";   // last rendered string (avoid redundant writes)

// Scroll state
static bool     _scrolling        = false;  // true while a scroll animation is running
static char     _scrollBuf[SCROLL_BUF_LEN] = "";
static uint32_t _scrollDeadlineMs = 0;      // 0 = no limit; >0 = keep looping until this millis()

// AP config message — stored and re-scrolled until station connects
static char     _apMsgBuf[48]    = "";      // "Config 192.168.4.1"

// Date display timer
static uint32_t _lastDateShow    = 0;   // millis() when date was last triggered

// ── Static/blink message state ──────────────────────────────────────────────────
static bool     _messageActive      = false;  // true while a static/blink message is running
static uint32_t _messageStartMs     = 0;      // when the static/blink message began
static uint32_t _messageBlinkLast   = 0;      // last blink toggle timestamp
static bool     _messageBlinkOn     = true;   // blink on/off state
static uint32_t _messagePhase1Ms    = 0;      // duration of the current message's blink/static phase (ms)

// ── Blink+Scroll state ────────────────────────────────────────────────────────
// Phase 1: blink the first screen of text for MESSAGE_BLINK_SCROLL_PHASE1_MS
//          (capped to whatever remains of the cycle's total duration).
// Phase 2: scroll the remainder of messageText.
// The whole blink→scroll cycle then repeats from phase 1 until messageDurationMs
// (tracked from _messageCycleStartMs, not from each phase's own start) elapses.
static bool     _blinkScrollPhase2  = false;  // true once we transition from blink to scroll
static bool     _blinkScrollCycling = false;  // true while a repeating BLINK_SCROLL cycle is active
static uint32_t _messageCycleStartMs  = 0;      // millis() when the overall BLINK_SCROLL cycle began

// ── Fixed date display state ──────────────────────────────────────────────────
// Shows "DAY DDMM" (or "DAY MMDD") all at once using the small 3×5 font.
// Duration is the full cfgDateIntervalMs; then we return to clock.
#define DATE_SHOW_MS  5000   // how long the date stays on screen (ms)

static bool     _dateActive       = false;
static uint32_t _dateMsStart      = 0;
static char     _dateLine[12]     = "";  // e.g. "SEG 1407" or "MON 0714"

// ── Auto brightness state (Sub-Task 6b) ──────────────────────────────────────
static bool     _lastNightActive   = false; // last evaluated night-window state

// ── Slot rotation state ───────────────────────────────────────────────────────
static bool     _slotActive       = false; // true while a non-clock slot is running
static int8_t   _forceSlotIndex   = -1;   // set by displayForceSlot(); consumed by displayTick()
static uint32_t _slotLastShownMs[4] = {0, 0, 0, 0}; // per-slot: millis() when it last finished showing

// ─── Internal helpers ─────────────────────────────────────────────────────────

// Resolve language index from cfgLocale string.
static uint8_t _langIndex() {
    return (cfgLocale[0] == 'p' || cfgLocale[0] == 'P') ? LANG_PT : LANG_EN;
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
    uint16_t speed = (messageScrollSpeedMs >= 0) ? (uint16_t)messageScrollSpeedMs : scrollSpeed;
    _startScrollAt(latin1Text, speed, durationMs);
}

// Apply the message's brightness override (if any) on top of currentBrightness.
static void _applyMessageBrightness() {
    if (messageBrightness >= 0) _display.setIntensity((uint8_t)messageBrightness);
}

// Restore the globally configured brightness after a message ends.
static void _restoreBrightness() {
    if (messageBrightness >= 0) _display.setIntensity(currentBrightness);
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

// Begin a static or blink message.
// isCycleStart: true when this call begins a brand-new BLINK_SCROLL cycle
// (as opposed to re-entering phase 1 after phase 2's scroll finished).
static void _startTimedMessage(bool isCycleStart = true) {
    _messageActive       = true;
    _messageStartMs      = millis();
    _messageBlinkLast    = millis();
    _messageBlinkOn      = true;
    _blinkScrollPhase2 = false;

    if (messageMode == MESSAGE_MODE_BLINK_SCROLL) {
        if (isCycleStart) _messageCycleStartMs = _messageStartMs;
        _blinkScrollCycling = true;
        // Phase 1 gets a fixed slice, capped to whatever remains of the
        // overall cycle budget (messageDurationMs, measured from cycle start).
        uint32_t cycleElapsed = _messageStartMs - _messageCycleStartMs;
        uint32_t cycleRemaining = (messageDurationMs > cycleElapsed) ? (messageDurationMs - cycleElapsed) : 0;
        _messagePhase1Ms = (cycleRemaining < MESSAGE_BLINK_SCROLL_PHASE1_MS)
                             ? cycleRemaining : MESSAGE_BLINK_SCROLL_PHASE1_MS;
    } else {
        _blinkScrollCycling = false;
        _messagePhase1Ms = messageDurationMs;
    }

    _applyMessageBrightness();
    _printCentered(messageText);
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
    // getTextColumns(day) alone omits the trailing inter-character spacing
    // that IS present when "day" is immediately followed by the separating
    // space in _dateLine — without adding it back, dayWidth undercounts by
    // 1px and the day name's leading column (e.g. the left edge of "S" in
    // "SEX") falls outside the shift loop below, leaving it unshifted.
    uint16_t dayWidth    = _display.getTextColumns(day) + _display.getCharSpacing();
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

// Clear the cached time string and reset alignment so the clock redraws from
// scratch on this displayTick() call. Consumes clockModeChangePending, which is
// set by the HTTP handler when cfgClockMode changes at runtime — the actual
// display write must happen here, from the loop() task, not from the handler's
// call stack (concurrent SPI access from the AsyncTCP task would race displayTick()).
static void _forceRedraw() {
    _lastTimeStr[0] = '\0';
    _display.setTextAlignment(PA_CENTER);
}

// ─── HH:MM:SS helpers ─────────────────────────────────────────────────────────────

// Every _dateSmallFont glyph is 3 columns wide (see date_font.h). Cleared
// before each draw regardless, as cheap insurance against any future glyph
// narrower than the cell leaving the previous digit's pixels behind.
#define SS_DIGIT_CELL_PX  3

// Write one decimal digit from _dateSmallFont at the given visual start column.
// _dateSmallFont PROGMEM layout: header = 'F', version, firstChar(32), lastChar(90),
// height (5 bytes total), then per-glyph entries: <width>, <col0..colN-1>.
// FC16_HW column mapping: raw col = 31 - visual col.
static void _writeSmallDigit(uint8_t visualCol, char digit) {
    const uint8_t firstChar = pgm_read_byte(&_dateSmallFont[2]);  // = 32
    uint16_t offset = 5;  // skip 5-byte header

    for (char c = (char)firstChar; c < digit; c++) {
        uint8_t w = pgm_read_byte(&_dateSmallFont[offset]);
        offset += 1 + w;
    }

    uint8_t width = pgm_read_byte(&_dateSmallFont[offset]);
    offset++;

    MD_MAX72XX* mx = _display.getGraphicObject();
    // Clear the full cell first — cheap insurance against any future glyph
    // narrower than the cell leaving the previous digit's pixels behind.
    for (uint8_t i = 0; i < SS_DIGIT_CELL_PX; i++)
        mx->setColumn((uint8_t)(31 - visualCol - i), 0);
    for (uint8_t i = 0; i < width; i++) {
        uint8_t colByte = pgm_read_byte(&_dateSmallFont[offset + i]);
        // Shift 1 physical row down (the font is normally 1 row from the top,
        // 2 from the bottom -- see date_font.h) so the seconds sit lower,
        // subscript-style, under the main HH:MM text instead of centred.
        mx->setColumn((uint8_t)(31 - visualCol - i), (uint8_t)(colByte << 1));
    }
}

// Where the small-font seconds block (tens digit, optional 1px inner gap,
// units digit) should go, given the actual pixel width of the just-printed
// HH:MM text. Default-font digit widths vary (e.g. '1' is 3px, most others
// are 5px), so HH:MM's width swings between 17 and 26px depending on which
// digits are showing -- a fixed start column overlaps the minute's last
// digit for most times. Prefers, in order: a 1px gap on both sides of SS
// (MM|SS and tens|units) > a 1px MM|SS gap only > no gaps at all. The
// widest HH:MM strings (26px) leave exactly 6 spare columns on the 32-column
// display -- just enough for the two 3px digits with no gap anywhere.
struct SSLayout { uint8_t start; uint8_t unitsOffset; };

static SSLayout _ssLayout(uint16_t hmWidthPx) {
    if (hmWidthPx + 1 + SS_DIGIT_CELL_PX + 1 + SS_DIGIT_CELL_PX <= DISPLAY_WIDTH_PX)
        return SSLayout{ (uint8_t)(hmWidthPx + 1), (uint8_t)(SS_DIGIT_CELL_PX + 1) };
    if (hmWidthPx + 1 + 2 * SS_DIGIT_CELL_PX <= DISPLAY_WIDTH_PX)
        return SSLayout{ (uint8_t)(hmWidthPx + 1), SS_DIGIT_CELL_PX };
    return SSLayout{ (uint8_t)(DISPLAY_WIDTH_PX - 2 * SS_DIGIT_CELL_PX), SS_DIGIT_CELL_PX };
}

// Render HH:MM (blinking colon, normal font, PA_LEFT) and overlay SS in small
// font right after it (see _ssLayout).
// _display.print() clears the full display first; setColumn() then overlays SS.
static void _renderHHMMSS(struct tm& t, bool colonVisible) {
    // ':' and ' ' are both 2px wide in this display's default font, so
    // swapping between them to blink never shifts MM or the SS start column.
    char hmBuf[8];
    snprintf(hmBuf, sizeof(hmBuf), colonVisible ? "%02d:%02d" : "%02d %02d",
             t.tm_hour, t.tm_min);

    _display.setFont(nullptr);
    _display.setTextAlignment(PA_LEFT);
    uint16_t hmWidth = _display.getTextColumns(hmBuf);
    _display.print(hmBuf);  // clears display, writes HH:MM at visual-left

    SSLayout ss = _ssLayout(hmWidth);
    _writeSmallDigit(ss.start,                  (char)('0' + t.tm_sec / 10));
    _writeSmallDigit(ss.start + ss.unitsOffset, (char)('0' + t.tm_sec % 10));
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

// Build the quotes display string into dst[dstLen].
// Format: "SYM1: 38.42 +1.23%  SYM2: 189.75 -0.45%" (or with * prefix if stale).
// Returns false if there is no cached data (slot should be skipped).
static bool _buildQuotesString(char* dst, size_t dstLen) {
    if (quoteCacheCount == 0) return false;

    dst[0] = '\0';
    if (quotesCacheStale) strncat(dst, "*", dstLen - strlen(dst) - 1);

    for (uint8_t i = 0; i < quoteCacheCount; i++) {
        if (!quoteCache[i].valid) continue;
        char priceStr[24];
        formatQuotePrice(quoteCache[i].price, cfgLocale, priceStr, sizeof(priceStr));
        char entry[56];
        snprintf(entry, sizeof(entry), "%s%s: %s %c%.2f%%",
                 (i > 0) ? "  " : "",
                 quoteCache[i].symbol,
                 priceStr,
                 (quoteCache[i].changePercent >= 0) ? '+' : '-',
                 fabsf(quoteCache[i].changePercent));
        strncat(dst, entry, dstLen - strlen(dst) - 1);
    }
    return true;
}

// Start a slot scroll immediately (shared by timer-driven and forced paths).
// Returns true if a scroll was started.
static bool _startSlotScroll(uint8_t slot) {
    if (slot == 2) {
        char buf[SCROLL_BUF_LEN];
        if (_buildWeatherString(buf, sizeof(buf))) {
            _slotActive  = true;
            activeSlot   = 2;
            _startScrollAt(buf, scrollSpeed);
            return true;
        }
    } else if (slot == 3) {
        char buf[SCROLL_BUF_LEN];
        if (_buildQuotesString(buf, sizeof(buf))) {
            _slotActive  = true;
            activeSlot   = 3;
            _startScrollAt(buf, scrollSpeed);
            return true;
        }
    }
    return false;
}

// Whether the given slot is within its configured daily time window and
// allowed on the current weekday.
// slotScheduleStartMin[slot] == slotScheduleEndMin[slot] means "no time restriction".
// start > end wraps past midnight (e.g. 22:00-06:00 covers the overnight window).
// slotScheduleDaysMask[slot] bit N (tm_wday: 0=Sunday..6=Saturday) gates the weekday.
static bool _slotInSchedule(uint8_t slot) {
    if (!ntpSynced) return false;   // epoch time gives wrong tm_wday
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);

    if (!(slotScheduleDaysMask[slot] & (1 << t.tm_wday))) return false;

    uint16_t start = slotScheduleStartMin[slot];
    uint16_t end   = slotScheduleEndMin[slot];
    if (start == end) return true;

    uint16_t nowMin = (uint16_t)(t.tm_hour * 60 + t.tm_min);
    if (start < end) return nowMin >= start && nowMin < end;
    return nowMin >= start || nowMin < end;   // wraps past midnight
}

// Check slot rotation timer and trigger next slot when due.
// Must be called from displayTick() only when the display is idle (not
// scrolling, not messaging, not showing date).
static void _slotRotationTick() {
    uint32_t now = millis();

    // If a slot scroll was active and has now finished, clear the active flag.
    if (_slotActive) {
        if (_scrolling) return;   // still scrolling — nothing to do yet
        _slotLastShownMs[activeSlot] = now;
        _slotActive = false;
        activeSlot  = 0;          // return to clock
        return;   // give clock one tick to redraw before next check
    }

    // Forced preview requested by displayForceSlot() (from HTTP handler).
    if (_forceSlotIndex >= 0) {
        uint8_t s = (uint8_t)_forceSlotIndex;
        _forceSlotIndex = -1;
        _startSlotScroll(s);   // ignore return — if no cache, just skip
        return;
    }

    // Timer-driven rotation: slot 2 = Weather, slot 3 = Quotes.
    // Each slot tracks its own "due" time independently so one slot's shorter
    // interval can never starve the other — whichever eligible slot has been
    // waiting longest past its own interval goes next.
    bool slot2Due = slotEnabled[2] && weatherCache.valid && _slotInSchedule(2) &&
                    (now - _slotLastShownMs[2] >= slotIntervalMs[2]);
    bool slot3Due = slotEnabled[3] && quoteCacheCount > 0 && _slotInSchedule(3) &&
                    (now - _slotLastShownMs[3] >= slotIntervalMs[3]);

    if (slot2Due && slot3Due) {
        // Both due — show whichever has been waiting longest (relative to its own interval).
        uint32_t overdue2 = (now - _slotLastShownMs[2]) - slotIntervalMs[2];
        uint32_t overdue3 = (now - _slotLastShownMs[3]) - slotIntervalMs[3];
        _startSlotScroll(overdue3 > overdue2 ? 3 : 2);
    } else if (slot2Due) {
        _startSlotScroll(2);
    } else if (slot3Due) {
        _startSlotScroll(3);
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
    _slotLastShownMs[2] = millis();   // don't show weather slot immediately on boot
    _slotLastShownMs[3] = millis();   // don't show quotes slot immediately on boot
}

void displayTick() {
    uint32_t now = millis();

    // ── Auto brightness by time of day (Sub-Task 6b) ──────────────────────────
    if (cfgNightBrightnessEnabled && ntpSynced) {
        time_t   now_t  = time(nullptr);
        struct tm* tm_l = localtime(&now_t);
        uint16_t nowMin = (uint16_t)(tm_l->tm_hour * 60 + tm_l->tm_min);
        bool nightActive;
        if (cfgNightStartMin == cfgNightEndMin) {
            nightActive = false;  // start == end → disabled sentinel
        } else if (cfgNightStartMin < cfgNightEndMin) {
            nightActive = (nowMin >= cfgNightStartMin && nowMin < cfgNightEndMin);
        } else {
            // window crosses midnight
            nightActive = (nowMin >= cfgNightStartMin || nowMin < cfgNightEndMin);
        }
        if (nightActive != _lastNightActive) {
            _lastNightActive = nightActive;
            if (messageBrightness < 0) {  // don't override active message brightness
                _display.setIntensity(nightActive ? cfgNightBrightnessLevel : currentBrightness);
            }
        }
    }

    if (clockModeChangePending) {
        clockModeChangePending = false;
        _forceRedraw();
    }

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
                uint32_t cycleElapsed = now - _messageCycleStartMs;
                if (cycleElapsed < messageDurationMs) {
                    _startTimedMessage(false);
                    return;
                }
                _blinkScrollCycling = false;
            }

            _restoreBrightness();
            messageBrightness    = -1;
            messageScrollSpeedMs = -1;

            // In AP mode, immediately re-scroll the config message
            if (isApMode() && _apMsgBuf[0] != '\0') {
                _startScrollAt(_apMsgBuf, AP_SCROLL_SPEED_MS);
                return;
            }
        } else {
            return;   // still scrolling — nothing else to do this tick
        }
    }

    // ── Static / blink message ──────────────────────────────────────────────────
    if (_messageActive) {
        uint32_t elapsed = now - _messageStartMs;
        if (elapsed >= _messagePhase1Ms) {
            // Blink phase done — if BLINK_SCROLL, transition to scroll phase.
            // Only scroll the part of the message that wasn't already shown
            // (blinking) during phase 1.
            if (messageMode == MESSAGE_MODE_BLINK_SCROLL && !_blinkScrollPhase2) {
                _blinkScrollPhase2 = true;
                size_t shown = _charsFittingOnScreen(messageText);
                if (messageText[shown] != '\0') {
                    _messageActive = false;
                    // Remainder of the overall cycle budget goes to phase 2.
                    uint32_t cycleElapsed = now - _messageCycleStartMs;
                    uint32_t remaining = (messageDurationMs > cycleElapsed) ? (messageDurationMs - cycleElapsed) : 0;
                    _startScroll(messageText + shown, remaining);
                    return;
                }
                // Whole message already fits on screen — nothing to scroll.
                // If the cycle still has time left, just restart phase 1 (re-blink)
                // instead of ending the message early.
                uint32_t cycleElapsed = now - _messageCycleStartMs;
                if (cycleElapsed < messageDurationMs) {
                    _startTimedMessage(false);
                    return;
                }
                _blinkScrollCycling = false;
            }
            // Message duration expired — return to clock
            _messageActive = false;
            _display.displayClear();
            _display.setTextAlignment(PA_CENTER);
            _restoreBrightness();
            messageBrightness    = -1;
            messageScrollSpeedMs = -1;
            _lastTimeStr[0] = '\0';
        } else if (messageMode == MESSAGE_MODE_BLINK || messageMode == MESSAGE_MODE_BLINK_SCROLL) {
            if (now - _messageBlinkLast >= MESSAGE_BLINK_PERIOD_MS) {
                _messageBlinkLast = now;
                _messageBlinkOn   = !_messageBlinkOn;
                _display.displayClear();
                _display.setTextAlignment(PA_CENTER);
                if (_messageBlinkOn) _display.print(messageText);
            }
        }
        // MESSAGE_MODE_STATIC: just leave the text on screen until duration expires
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

    // ── Check for pending message (one-shot, highest priority after scroll) ──────
    if (messagePending) {
        messagePending = false;
        _blinkScrollPhase2 = false;
        if (messageMode == MESSAGE_MODE_SCROLL) {
            _applyMessageBrightness();
            _startScroll(messageText, messageDurationMs);
        } else {
            // MESSAGE_MODE_BLINK, MESSAGE_MODE_STATIC, MESSAGE_MODE_BLINK_SCROLL all start timed
            _startTimedMessage();
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

    // ── Slot rotation (weather + quotes slots) ────────────────────────────────
    _slotRotationTick();
    if (_scrolling) return;   // rotation just started a scroll — done this tick

    if (cfgClockMode == CLOCK_MODE_HHMMSS) {
        // Seconds mode: HH:MM left-aligned with STEADY colon + SS in small font.
        // The colon never blinks — _lastBlink is reused as a 1-second refresh tick.
        if (now - _lastBlink >= BLINK_INTERVAL_MS) {
            _lastBlink = now;

            if (!ntpSynced) {
                _display.setFont(nullptr);
                _display.setTextAlignment(PA_LEFT);
                const char* buf = "--:--";
                uint16_t hmWidth = _display.getTextColumns(buf);
                _display.print(buf);  // clears display, writes "--:--" at visual-left
                // No real time to show yet — blank the seconds block instead of drawing digits.
                SSLayout ss = _ssLayout(hmWidth);
                uint8_t ssEnd = ss.start + ss.unitsOffset + SS_DIGIT_CELL_PX;
                MD_MAX72XX* mx = _display.getGraphicObject();
                for (uint8_t vc = ss.start; vc < ssEnd; vc++)
                    mx->setColumn((uint8_t)(31 - vc), 0);
            } else {
                struct tm timeinfo;
                if (getLocalTime(&timeinfo)) {
                    _renderHHMMSS(timeinfo, true);  // colon always visible in HH:MM:SS mode
                }
            }
        }
    } else {
        // ── HH:MM mode: colon blinks every BLINK_INTERVAL_MS ─────────────────
        if (now - _lastBlink >= BLINK_INTERVAL_MS) {
            _lastBlink    = now;
            _colonVisible = !_colonVisible;

            char buf[10];
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

            if (strcmp(buf, _lastTimeStr) != 0) {
                strncpy(_lastTimeStr, buf, sizeof(_lastTimeStr) - 1);
                _lastTimeStr[sizeof(_lastTimeStr) - 1] = '\0';
                _display.setFont(nullptr);
                _display.setTextAlignment(PA_CENTER);
                _display.print(buf);
            }
        }
    }
}

void setBrightness(uint8_t level) {
    currentBrightness = level;
    _display.setIntensity(level);
    // Force re-evaluation on next displayTick() so night mode re-asserts
    // immediately if the window is still active after a manual brightness change.
    _lastNightActive = false;
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

