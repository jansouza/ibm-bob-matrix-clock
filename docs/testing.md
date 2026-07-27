# Testing — Smart Matrix Clock

This document describes the native host test suite that ships alongside the firmware.

---

## Overview

The firmware targets ESP32 hardware, which means most modules depend on Arduino/ESP32 runtime APIs that only exist on-device. However, a significant portion of the logic — encoding, locale tables, scheduling algorithms, data parsing — is **pure C/C++ with no hardware dependency**. These modules are compiled and tested directly on the development host (Linux or macOS) using plain `g++`.

| | Host suite | On-device |
|---|---|---|
| No hardware needed | ✅ | ❌ |
| Run time | < 1 s | requires flash + reboot |
| Covers | pure-logic modules | everything |
| Command | `cd tests && make` | flash + serial monitor |

---

## Running the tests

```bash
cd tests
make          # build + run all tests
make build    # build only (no run)
make clean    # remove build artefacts
```

The binary is built at `tests/build/run_tests` and exits with code `0` on all-pass, `1` on any failure.

### Expected output (all-pass)

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Running Smart Matrix Clock native tests
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

▸ utf8ToLatin1 — ASCII passthrough
  6/6 passed
...
▸ config constants — compile-time sanity
  21/21 passed

✓ ALL PASS  Total: 251 passed, 0 failed out of 251 tests
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### Example failure output

```
▸ expandIconTags — known icon names
  FAIL  [expandIconTags — known icon names] [heart] -> 0x03
         tests/test_text_encoding.cpp:268: STREQ failed: "" != "\x03"
  10/11 passed  (1 FAILED)

✗ FAILURES  Total: 250 passed, 1 failed out of 251 tests
```

---

## Test suite structure

```
tests/
├── Makefile                        ← native host build (g++ -std=c++17)
├── framework.h                     ← SUITE / TEST / ASSERT_* macros
├── runner.cpp                      ← defines RUNNER_MAIN → generates main()
├── stubs/
│   ├── arduino_stub.h              ← replaces Arduino/ESP32 headers (PROGMEM, Serial, …)
│   └── stubs.cpp                   ← single definition point for suite registry + test state
├── test_text_encoding.cpp
├── test_locale_data.cpp
├── test_slot_schedule.cpp
├── test_data_fetcher_split.cpp
└── test_persistence_language.cpp
```

---

## Coverage by file

### `test_text_encoding.cpp` — 76 tests

Tests for [`text_encoding.h/cpp`](../smart-matrix-clock-esp32/text_encoding.h).

| Suite | Description |
|---|---|
| `utf8ToLatin1 — ASCII passthrough` | Empty, pure ASCII, truncation to `maxLen` |
| `utf8ToLatin1 — 2-byte UTF-8 sequences` | U+00E9 (é), U+00C0 (À), U+00B0 (°), U+00FC (ü), mixed ASCII+accented |
| `utf8ToLatin1 — code-points beyond Latin-1` | U+0100, U+20AC (€), 4-byte emoji → `?`; surrounding text preserved |
| `utf8ToLatin1 — malformed sequences` | Lone continuation byte, truncated 2-byte sequence |
| `latin1ToUtf8 — ASCII passthrough` | Empty, pure ASCII |
| `latin1ToUtf8 — high Latin-1 bytes` | 0xE9, 0xB0, 0x80, 0xFF encoding |
| `latin1ToUtf8 — roundtrip with utf8ToLatin1` | ASCII, single byte, mixed, all 128 high bytes |
| `latin1ToUtf8 — buffer boundary` | Null-termination within `maxLen` |
| `expandIconTags — known icon names` | All 11 icons: heart, diamond, spade, bullet, star, arrow\_right, arrow\_left, up, down, bell, warn |
| `expandIconTags — mixed text and tags` | Prefix, suffix, multiple tags, tags surrounded by text |
| `expandIconTags — unknown tags pass through verbatim` | Unknown name, wrong case, empty `[]`, unclosed `[`, no tags |
| `expandIconTags — buffer boundary` | Null-termination, `maxLen=1` |
| `expandIconTags — null / empty guards` | `nullptr` src, empty string |

### `test_locale_data.cpp` — 82 tests

Tests for [`locale_data.h/cpp`](../smart-matrix-clock-esp32/locale_data.h).

| Suite | Description |
|---|---|
| `localeDayName — English` | All 7 days (Sun–Sat), overflow wraps to Sunday |
| `localeDayName — Portuguese` | All 7 days |
| `localeDayName — unknown language` | Falls back to English |
| `localeMonthName — English` | All 12 months, overflow wraps to January |
| `localeMonthName — Portuguese` | Spot checks (FEV, ABR, MAI, AGO, SET, OUT, DEZ) |
| `ianaToPostfix — known zones` | UTC, São Paulo (BRT prefix), Fortaleza, New York, Los Angeles, Paris, Tokyo, Kolkata, Manaus |
| `ianaToPostfix — unknown / edge cases` | Unknown zone, empty string, `nullptr`, partial match, wrong case |
| `tzTableSize / tzTableEntry` | ≥ 10 entries, non-empty fields, unique IANA names, entry matches `ianaToPostfix()` result |
| `weatherConditionName — English` | Codes 0, 2, 45, 61, 65, 71, 95, 99; unknown code fallback |
| `weatherConditionName — Portuguese` | Codes 0, 2, 45, 63, 80, 95; unknown code fallback |
| `weatherConditionName — unknown language` | Falls back to English |

### `test_slot_schedule.cpp` — 53 tests

Covers the scheduling algorithm used in [`display.cpp`](../smart-matrix-clock-esp32/display.cpp)'s `_slotInSchedule()` and `displayTick()`'s night-brightness window check. The algorithm is replicated as a pure function in the test file (see note on private functions below).

| Suite | Description |
|---|---|
| `slotInSchedule — same-day window [start, end)` | At start (inside), at start+1 (inside), mid (inside), end-1 (inside), at end (outside, half-open), beyond end (outside), midnight (outside) |
| `slotInSchedule — start == end means always active` | `0,0`, `480,480`, `1439,1439` with various times |
| `slotInSchedule — window crosses midnight (start > end)` | Default 23:00–07:00 night window: at 23:00 (in), at midnight (in), at 03:00 (in), at 06:59 (in), at 07:00 (out, half-open), at noon (out), at 22:59 (out) |
| `slotInSchedule — day mask gating` | All-days mask, Sunday-only, Monday-only, Saturday-only, Mon–Fri (0x3E), mask 0x00 (never), combined window+mask |
| `slotInSchedule — boundary minute values` | Window `[0,1439)`, 1-minute window, quotes default schedule on Mon (in) / Sat (out) / Mon 19:00 (out) |
| `nightWindowActive — same-day window` | `start == end` disabled, inside `[600,900)`, outside |
| `nightWindowActive — midnight-crossing window` | Default 23:00–07:00: 22:59 (day), 23:00 (night), midnight (night), 07:00 (day, half-open), noon (day) |

### `test_data_fetcher_split.cpp` — 23 tests

Covers the ticker-splitting algorithm from [`data_fetcher.cpp`](../smart-matrix-clock-esp32/data_fetcher.cpp)'s `_splitTickers()`.

| Suite | Description |
|---|---|
| `splitTickers — basic splitting` | Empty → 0, single, two, three, max-8 cap, 9th silently dropped |
| `splitTickers — whitespace trimming` | Leading space, trailing space, both, spaces around comma, multiple leading |
| `splitTickers — empty tokens` | Leading comma, trailing comma, consecutive commas, whitespace-only token, all commas |
| `splitTickers — symbol length safety` | Long symbol truncated to `QUOTES_SYMBOL_MAX`; exact-length symbol survives |
| `splitTickers — realistic inputs` | Brazilian `.SA` tickers, mix of US stock + crypto + forex |

### `test_persistence_language.cpp` — 43 tests

Covers `isUiLanguageValid()` from [`persistence.cpp`](../smart-matrix-clock-esp32/persistence.cpp), `formatQuotePrice()` from `text_encoding.cpp`, and compile-time sanity of all constants in [`config.h`](../smart-matrix-clock-esp32/config.h).

| Suite | Description |
|---|---|
| `isUiLanguageValid — accepted codes` | `"en"`, `"pt"` |
| `isUiLanguageValid — rejected codes` | Uppercase, unknown language, empty string, `nullptr`, partial |
| `formatQuotePrice — English locale` | No-separator (< 1 000), 1 234.50, 12 345.00, 1 234 567.00, zero, 0.01, negative, null/unknown locale fallback |
| `formatQuotePrice — Portuguese locale` | Same set with `,` decimal / `.` thousands; `"PT"` uppercase accepted |
| `formatQuotePrice — buffer boundary` | Null-termination, `maxLen=1`, `maxLen=5` |
| `config constants — compile-time sanity` | 21 checks: brightness/scroll-speed ranges, buffer sizes, NTP re-sync ≥ 1 min, clock mode values, slot constants, schedule limits, NVS namespace non-empty, **all 37 NVS keys ≤ 15 chars** (ESP32 NVS hard limit) |

---

## Framework reference

The test framework is self-contained in [`tests/framework.h`](../tests/framework.h) — no external dependencies.

### Macros

| Macro | Usage |
|---|---|
| `SUITE("name") { ... }` | Declares and auto-registers a test suite (global static constructor) |
| `TEST("desc") { ... }` | Named test case inside a suite — each `TEST` is an independent pass/fail |
| `ASSERT_EQ(a, b)` | Equality — prints both values on failure |
| `ASSERT_NE(a, b)` | Inequality |
| `ASSERT_TRUE(expr)` | Truthy |
| `ASSERT_FALSE(expr)` | Falsy |
| `ASSERT_STREQ(s1, s2)` | `strcmp(s1, s2) == 0` — prints both strings on failure |
| `ASSERT_STRNE(s1, s2)` | `strcmp(s1, s2) != 0` |
| `ASSERT_NEAR(a, b, eps)` | `|a − b| ≤ eps` — for floating-point comparisons |

Multiple `ASSERT_*` calls are allowed inside one `TEST` block. A `TEST` is marked **FAIL** if any assertion in it fails; it continues executing the rest of the test body.

---

## Modules NOT covered by host tests

The following modules require Arduino/ESP32 runtime APIs and can only be validated on-device:

| Module | Dependency |
|---|---|
| `display.cpp` | `MD_Parola` / `MD_MAX72XX` hardware objects, `SPI.h` |
| `wifi_manager.cpp` | `WiFi.*`, `ESP.restart()` |
| `ntp.cpp` | `configTime()`, `getLocalTime()` |
| `persistence.cpp` | `Preferences` (NVS flash access) |
| `web_routes.cpp` | `ESPAsyncWebServer`, `WiFi.localIP()` |
| `data_fetcher.cpp` | `HTTPClient`, live network |

For these, validation is done by flashing the firmware and observing the serial monitor and web panel.

---

## Adding tests for new logic

### Rule of thumb

If a function you add or change contains **no `#include <Arduino.h>`**, **no `WiFi.*`**, **no `Preferences`**, **no `HTTPClient`**, **no display objects** — it is testable on the host.

### Step-by-step

1. **Identify the function.** Pure-logic helpers in `text_encoding.cpp`, `locale_data.cpp`, or algorithms extracted inline from `display.cpp` / `data_fetcher.cpp` are all candidates.

2. **Locate the test file.** If the function lives in `text_encoding.cpp` → add tests to `test_text_encoding.cpp`. For a new module, create `test_<module>.cpp`.

3. **Register a new file** in [`tests/Makefile`](../tests/Makefile) `TEST_SRCS`:
   ```make
   TEST_SRCS := \
       test_text_encoding.cpp \
       test_locale_data.cpp \
       ...
       test_my_new_module.cpp \   ← add here
       runner.cpp
   ```

4. **Write the test file:**
   ```cpp
   #include "framework.h"
   #include "my_module.h"   // or replicate the algorithm inline

   SUITE("myFunc — normal inputs") {
       char dst[64];

       TEST("empty input produces empty output") {
           myFunc("", dst, sizeof(dst));
           ASSERT_STREQ(dst, "");
       }

       TEST("known value maps correctly") {
           ASSERT_EQ(myFunc(42), 100);
       }
   }

   SUITE("myFunc — edge cases") {
       TEST("null input returns dst") {
           char dst[4];
           char* r = myFunc(nullptr, dst, sizeof(dst));
           ASSERT_TRUE(r == dst);
       }
   }
   ```

5. **For private (`static`) functions:** replicate the algorithm verbatim in the test file and add a comment:
   ```cpp
   // Algorithm replica from display.cpp / _slotInSchedule().
   // Keep in sync if the original changes.
   static bool slotInSchedule(...) { ... }
   ```

6. **Run and verify:**
   ```bash
   cd tests && make
   # → ✓ ALL PASS  Total: N passed, 0 failed
   ```

7. **Also compile the firmware** to ensure no firmware source was accidentally touched:
   ```bash
   arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32
   ```

### Test writing guidelines

- One `TEST` per distinct behaviour — not one test per function.
- Test boundary values explicitly: empty input, `maxLen=1`, zero, negative, `nullptr`.
- Use `ASSERT_STREQ` for strings and `ASSERT_NEAR` for floats; use `ASSERT_EQ` with casts for byte values (`ASSERT_EQ((uint8_t)dst[0], 0x03u)`).
- Name suites as `"functionName — category"` (e.g. `"expandIconTags — unknown tags pass through verbatim"`).
- Keep each SUITE under ~15 tests — split into multiple suites if it grows larger.

---

## Validation checklist (before committing any code change)

```bash
# 1. Run all host tests — must finish ✓ ALL PASS
cd tests && make

# 2. Compile the firmware — must succeed with no new warnings
arduino-cli compile --fqbn esp32:esp32:esp32 smart-matrix-clock-esp32
```

Both steps are required. The tests alone don't catch firmware regressions; the compile step doesn't catch logic bugs in pure-logic modules.
