#pragma once
#include <stdint.h>

// ─── Supported languages ──────────────────────────────────────────────────────
// Language codes used in configuration (stored in NVS as "pt" or "en").
#define LANG_PT  0
#define LANG_EN  1
#define LANG_COUNT 2

// Abbreviated weekday name by language.
// dayIndex: 0=Sunday … 6=Saturday (tm_wday convention).
const char* localeDayName(uint8_t lang, uint8_t dayIndex);

// Abbreviated month name by language.
// monthIndex: 0=January … 11=December (tm_mon convention).
const char* localeMonthName(uint8_t lang, uint8_t monthIndex);

// ─── IANA timezone → POSIX TZ string ─────────────────────────────────────────

typedef struct {
    const char* iana;   // IANA timezone name (e.g. "America/Sao_Paulo")
    const char* posix;  // POSIX TZ string     (e.g. "BRT3BRST2,M10.3.0/0,M2.3.0/0")
} TZEntry;

// Returns the POSIX TZ string for the given IANA name.
// Returns nullptr if not found.
const char* ianaToPostfix(const char* iana);

// Returns total number of entries in the table.
uint8_t tzTableSize();

// Returns the TZEntry at index i (for enumeration by the web UI).
const TZEntry* tzTableEntry(uint8_t i);
