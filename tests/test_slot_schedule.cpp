/*
 * test_slot_schedule.cpp
 *
 * Tests for the slot-scheduling logic extracted from display.cpp's
 * _slotInSchedule() function.  The algorithm is re-implemented here as a
 * pure function (no hardware deps) so we can drive it with synthetic time
 * values and verify the window/wrap/day-mask rules in isolation.
 *
 * The function under test (_slotInSchedule) is private in display.cpp, so
 * we replicate its exact algorithm here — any change to the algorithm in
 * display.cpp must be reflected here to keep the tests meaningful.
 */

#include "framework.h"
#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// ─── Algorithm replica from display.cpp / _slotInSchedule() ──────────────────
// Parameters:
//   startMin, endMin  — schedule window in minutes-of-day (0-1439)
//   daysMask          — bitmask of enabled tm_wday values (0=Sunday)
//   nowMin            — current time in minutes-of-day
//   wday              — current tm_wday (0=Sunday..6=Saturday)
//
// Returns true if the current time falls within the window AND the current
// weekday is enabled.
//
// Semantics from AGENTS.md:
//   start == end  → no restriction (always active, pending the days mask)
//   start < end   → same-day window [start, end)
//   start > end   → window crosses midnight (start..23:59] ∪ [0..end)

static bool slotInSchedule(uint16_t startMin, uint16_t endMin,
                            uint8_t  daysMask,
                            uint16_t nowMin,   uint8_t wday) {
    // Day-mask gate
    if (!(daysMask & (1 << wday))) return false;

    // No time restriction (start == end)
    if (startMin == endMin) return true;

    if (startMin < endMin) {
        return (nowMin >= startMin && nowMin < endMin);
    }
    // Crosses midnight
    return (nowMin >= startMin || nowMin < endMin);
}

// ─── Same-day window tests ────────────────────────────────────────────────────

SUITE("slotInSchedule — same-day window [start, end)") {
    // Window 09:00–18:00 (540–1080), all days enabled
    const uint16_t start = 540, end = 1080;
    const uint8_t  all   = 0x7F;

    TEST("time exactly at start is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 540, 1));
    }
    TEST("time 1 minute after start is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 541, 1));
    }
    TEST("time in the middle (13:00) is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 780, 3));
    }
    TEST("time at end - 1 is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 1079, 5));
    }
    TEST("time exactly at end is outside (half-open interval)") {
        ASSERT_FALSE(slotInSchedule(start, end, all, 1080, 1));
    }
    TEST("time 1 minute after end is outside") {
        ASSERT_FALSE(slotInSchedule(start, end, all, 1081, 1));
    }
    TEST("midnight (0) is outside") {
        ASSERT_FALSE(slotInSchedule(start, end, all, 0, 2));
    }
    TEST("23:59 is outside") {
        ASSERT_FALSE(slotInSchedule(start, end, all, 1439, 4));
    }
}

SUITE("slotInSchedule — start == end means always active") {
    const uint8_t all = 0x7F;

    TEST("0,0 with midnight is true") {
        ASSERT_TRUE(slotInSchedule(0, 0, all, 0, 0));
    }
    TEST("480,480 at noon is true") {
        ASSERT_TRUE(slotInSchedule(480, 480, all, 720, 3));
    }
    TEST("1439,1439 at 23:58 is true") {
        ASSERT_TRUE(slotInSchedule(1439, 1439, all, 1438, 6));
    }
}

// ─── Midnight-crossing window tests ──────────────────────────────────────────

SUITE("slotInSchedule — window crosses midnight (start > end)") {
    // Window 23:00–07:00 (1380–420), all days enabled
    const uint16_t start = 1380, end = 420;
    const uint8_t  all   = 0x7F;

    TEST("time at start (23:00) is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 1380, 1));
    }
    TEST("time at 23:30 is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 1410, 1));
    }
    TEST("time at 23:59 is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 1439, 1));
    }
    TEST("time at midnight (0) is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 0, 2));
    }
    TEST("time at 03:00 is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 180, 2));
    }
    TEST("time at end - 1 (06:59) is inside") {
        ASSERT_TRUE(slotInSchedule(start, end, all, 419, 2));
    }
    TEST("time at end (07:00) is outside (half-open)") {
        ASSERT_FALSE(slotInSchedule(start, end, all, 420, 2));
    }
    TEST("time at 12:00 is outside") {
        ASSERT_FALSE(slotInSchedule(start, end, all, 720, 3));
    }
    TEST("time at 22:59 is outside") {
        ASSERT_FALSE(slotInSchedule(start, end, all, 1379, 5));
    }
}

// ─── Day-mask tests ───────────────────────────────────────────────────────────

SUITE("slotInSchedule — day mask gating") {
    // Window always open (start == end) so the day mask is the only gate
    const uint16_t s = 0, e = 0;
    const uint16_t noon = 720;

    TEST("all days enabled (0x7F) — every wday passes") {
        for (uint8_t d = 0; d <= 6; d++) {
            ASSERT_TRUE(slotInSchedule(s, e, 0x7F, noon, d));
        }
    }
    TEST("Sunday-only mask (bit 0) — only wday 0 passes") {
        ASSERT_TRUE(slotInSchedule(s, e, 0x01, noon, 0));
        for (uint8_t d = 1; d <= 6; d++) {
            ASSERT_FALSE(slotInSchedule(s, e, 0x01, noon, d));
        }
    }
    TEST("Monday-only mask (bit 1)") {
        ASSERT_TRUE(slotInSchedule(s, e, 0x02, noon, 1));
        ASSERT_FALSE(slotInSchedule(s, e, 0x02, noon, 0));
        ASSERT_FALSE(slotInSchedule(s, e, 0x02, noon, 2));
    }
    TEST("Saturday-only mask (bit 6)") {
        ASSERT_TRUE(slotInSchedule(s, e, 0x40, noon, 6));
        ASSERT_FALSE(slotInSchedule(s, e, 0x40, noon, 0));
    }
    TEST("weekday mask Mon-Fri = 0b0111110 = 0x3E") {
        // Mon-Fri: wdays 1-5
        const uint8_t monFri = (1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5);  // = 0x3E
        ASSERT_FALSE(slotInSchedule(s, e, monFri, noon, 0));  // Sun
        for (uint8_t d = 1; d <= 5; d++) {
            ASSERT_TRUE(slotInSchedule(s, e, monFri, noon, d));
        }
        ASSERT_FALSE(slotInSchedule(s, e, monFri, noon, 6));  // Sat
    }
    TEST("mask 0x00 — never active") {
        for (uint8_t d = 0; d <= 6; d++) {
            ASSERT_FALSE(slotInSchedule(s, e, 0x00, noon, d));
        }
    }
    TEST("day mask + same-day window combined") {
        // Window 09:00–18:00, Mon-Fri only
        const uint16_t ws = 540, we = 1080;
        const uint8_t  mf = (1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5);
        // Monday at 13:00 — inside window AND enabled day
        ASSERT_TRUE(slotInSchedule(ws, we, mf, 780, 1));
        // Monday at 20:00 — outside window, enabled day
        ASSERT_FALSE(slotInSchedule(ws, we, mf, 1200, 1));
        // Sunday at 13:00 — inside window but disabled day
        ASSERT_FALSE(slotInSchedule(ws, we, mf, 780, 0));
    }
}

// ─── Edge / boundary values ───────────────────────────────────────────────────

SUITE("slotInSchedule — boundary minute values") {
    const uint8_t all = 0x7F;

    TEST("window 0–1439 with nowMin=0 is inside") {
        ASSERT_TRUE(slotInSchedule(0, 1439, all, 0, 1));
    }
    TEST("window 0–1439 with nowMin=1438 is inside") {
        ASSERT_TRUE(slotInSchedule(0, 1439, all, 1438, 1));
    }
    TEST("window 0–1439 with nowMin=1439 is outside (half-open)") {
        ASSERT_FALSE(slotInSchedule(0, 1439, all, 1439, 1));
    }
    TEST("1-minute window [600, 601) — only 600 is inside") {
        ASSERT_TRUE(slotInSchedule(600, 601, all, 600, 1));
        ASSERT_FALSE(slotInSchedule(600, 601, all, 601, 1));
        ASSERT_FALSE(slotInSchedule(600, 601, all, 599, 1));
    }
    TEST("QUOTES default schedule: 480-1080, Mon-Fri") {
        const uint8_t mf = QUOTES_SCHED_DAYS_DEFAULT;
        // Monday 09:00 inside
        ASSERT_TRUE(slotInSchedule(QUOTES_SCHED_START_DEFAULT_MIN,
                                    QUOTES_SCHED_END_DEFAULT_MIN,
                                    mf, 540, 1));
        // Saturday 09:00 outside (day mask)
        ASSERT_FALSE(slotInSchedule(QUOTES_SCHED_START_DEFAULT_MIN,
                                     QUOTES_SCHED_END_DEFAULT_MIN,
                                     mf, 540, 6));
        // Monday 19:00 outside (time window)
        ASSERT_FALSE(slotInSchedule(QUOTES_SCHED_START_DEFAULT_MIN,
                                     QUOTES_SCHED_END_DEFAULT_MIN,
                                     mf, 1140, 1));
    }
}

// ─── Night-window algorithm (from displayTick's auto-brightness block) ─────────
// Same window + wrapping logic — tests drive displayTick's condition inline.

static bool nightWindowActive(uint16_t start, uint16_t end, uint16_t nowMin) {
    if (start == end) return false;          // disabled sentinel
    if (start < end)  return (nowMin >= start && nowMin < end);
    return (nowMin >= start || nowMin < end); // wraps midnight
}

SUITE("nightWindowActive — same-day window") {
    TEST("start == end is always disabled") {
        ASSERT_FALSE(nightWindowActive(720, 720, 720));
        ASSERT_FALSE(nightWindowActive(0, 0, 0));
    }
    TEST("inside window [600,900)") {
        ASSERT_TRUE(nightWindowActive(600, 900, 600));
        ASSERT_TRUE(nightWindowActive(600, 900, 750));
        ASSERT_TRUE(nightWindowActive(600, 900, 899));
    }
    TEST("outside window [600,900)") {
        ASSERT_FALSE(nightWindowActive(600, 900, 599));
        ASSERT_FALSE(nightWindowActive(600, 900, 900));
        ASSERT_FALSE(nightWindowActive(600, 900, 1000));
    }
}

SUITE("nightWindowActive — midnight-crossing window") {
    // Default: 23:00 (1380) – 07:00 (420)
    const uint16_t NS = DEFAULT_NIGHT_START_MIN;  // 1380
    const uint16_t NE = DEFAULT_NIGHT_END_MIN;    // 420

    TEST("22:59 is daytime") {
        ASSERT_FALSE(nightWindowActive(NS, NE, 1379));
    }
    TEST("23:00 is night") {
        ASSERT_TRUE(nightWindowActive(NS, NE, 1380));
    }
    TEST("midnight is night") {
        ASSERT_TRUE(nightWindowActive(NS, NE, 0));
    }
    TEST("03:00 is night") {
        ASSERT_TRUE(nightWindowActive(NS, NE, 180));
    }
    TEST("07:00 is daytime (half-open end)") {
        ASSERT_FALSE(nightWindowActive(NS, NE, 420));
    }
    TEST("12:00 is daytime") {
        ASSERT_FALSE(nightWindowActive(NS, NE, 720));
    }
}
