/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include "locale_data.h"
#include <string.h>

// ─── Day names ────────────────────────────────────────────────────────────────
// Index: 0=Sun 1=Mon 2=Tue 3=Wed 4=Thu 5=Fri 6=Sat

static const char* _daysEN[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
static const char* _daysPT[7] = { "DOM", "SEG", "TER", "QUA", "QUI", "SEX", "SAB" };

const char* localeDayName(uint8_t lang, uint8_t dayIndex) {
    if (dayIndex > 6) dayIndex = 0;
    if (lang == LANG_PT) return _daysPT[dayIndex];
    return _daysEN[dayIndex];
}

// ─── Month names ──────────────────────────────────────────────────────────────
// Index: 0=Jan … 11=Dec

static const char* _monthsEN[12] = {
    "JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"
};
static const char* _monthsPT[12] = {
    "JAN","FEV","MAR","ABR","MAI","JUN","JUL","AGO","SET","OUT","NOV","DEZ"
};

const char* localeMonthName(uint8_t lang, uint8_t monthIndex) {
    if (monthIndex > 11) monthIndex = 0;
    if (lang == LANG_PT) return _monthsPT[monthIndex];
    return _monthsEN[monthIndex];
}

// ─── IANA → POSIX table ───────────────────────────────────────────────────────
// Minimum 20 entries covering Brazil + UTC + major US/EU zones.
// POSIX TZ strings include DST rules where applicable.

static const TZEntry _tzTable[] = {
    // ── Brazil ────────────────────────────────────────────────────────────────
    { "America/Sao_Paulo",      "BRT3BRST2,M10.3.0/0,M2.3.0/0"   },
    { "America/Fortaleza",      "BRT3"                            },
    { "America/Recife",         "BRT3"                            },
    { "America/Bahia",          "BRT3"                            },
    { "America/Belem",          "BRT3"                            },
    { "America/Manaus",         "AMT4"                            },
    { "America/Porto_Velho",    "AMT4"                            },
    { "America/Boa_Vista",      "AMT4"                            },
    { "America/Rio_Branco",     "ACT5"                            },
    { "America/Noronha",        "FNT2"                            },
    // ── UTC ───────────────────────────────────────────────────────────────────
    { "UTC",                    "UTC0"                            },
    // ── US ────────────────────────────────────────────────────────────────────
    { "America/New_York",       "EST5EDT,M3.2.0,M11.1.0"         },
    { "America/Chicago",        "CST6CDT,M3.2.0,M11.1.0"         },
    { "America/Denver",         "MST7MDT,M3.2.0,M11.1.0"         },
    { "America/Los_Angeles",    "PST8PDT,M3.2.0,M11.1.0"         },
    { "America/Anchorage",      "AKST9AKDT,M3.2.0,M11.1.0"       },
    { "Pacific/Honolulu",       "HST10"                           },
    // ── Europe ────────────────────────────────────────────────────────────────
    { "Europe/London",          "GMT0BST-1,M3.5.0/1,M10.5.0"     },
    { "Europe/Paris",           "CET-1CEST,M3.5.0,M10.5.0/3"     },
    { "Europe/Berlin",          "CET-1CEST,M3.5.0,M10.5.0/3"     },
    { "Europe/Helsinki",        "EET-2EEST,M3.5.0/3,M10.5.0/4"   },
    { "Europe/Moscow",          "MSK-3"                           },
    // ── Asia / Pacific ────────────────────────────────────────────────────────
    { "Asia/Dubai",             "GST-4"                           },
    { "Asia/Kolkata",           "IST-5:30"                        },
    { "Asia/Tokyo",             "JST-9"                           },
    { "Australia/Sydney",       "AEST-10AEDT,M10.1.0,M4.1.0/3"   },
};

static const uint8_t _tzCount = sizeof(_tzTable) / sizeof(_tzTable[0]);

const char* ianaToPostfix(const char* iana) {
    if (!iana) return nullptr;
    for (uint8_t i = 0; i < _tzCount; i++) {
        if (strcmp(_tzTable[i].iana, iana) == 0) {
            return _tzTable[i].posix;
        }
    }
    return nullptr;
}

uint8_t tzTableSize() {
    return _tzCount;
}

const TZEntry* tzTableEntry(uint8_t i) {
    if (i >= _tzCount) return nullptr;
    return &_tzTable[i];
}

// ─── WMO weather condition code → localised string ────────────────────────────
// WMO code groups (from Open-Meteo / WMO Table 4677):
//   0        Clear sky
//   1,2,3    Mainly clear / partly cloudy / overcast
//   45,48    Fog / depositing rime fog
//   51,53,55 Drizzle light/moderate/dense
//   56,57    Freezing drizzle light/heavy
//   61,63,65 Rain light/moderate/heavy
//   66,67    Freezing rain light/heavy
//   71,73,75 Snow light/moderate/heavy
//   77       Snow grains
//   80,81,82 Rain showers slight/moderate/heavy
//   85,86    Snow showers slight/heavy
//   95       Thunderstorm
//   96,99    Thunderstorm w/ hail

struct _WMOEntry {
    uint8_t     code;
    const char* en;
    const char* pt;
};

static const _WMOEntry _wmoTable[] = {
    {  0, "Clear",        "Limpo"       },
    {  1, "Mainly clear", "Qtdo limpo"  },
    {  2, "Part cloudy",  "Nublado"     },
    {  3, "Overcast",     "Coberto"     },
    { 45, "Fog",          "Nevoa"       },
    { 48, "Rime fog",     "Geada"       },
    { 51, "Lt drizzle",   "Garoa leve"  },
    { 53, "Drizzle",      "Garoa"       },
    { 55, "Hvy drizzle",  "Garoa forte" },
    { 56, "Frz drizzle",  "Garoa fria"  },
    { 57, "Hvy frz drz",  "Garoa g fria"},
    { 61, "Lt rain",      "Chuva leve"  },
    { 63, "Rain",         "Chuva"       },
    { 65, "Hvy rain",     "Chuva forte" },
    { 66, "Frz rain",     "Chuva fria"  },
    { 67, "Hvy frz rain", "Chv g fria"  },
    { 71, "Lt snow",      "Neve leve"   },
    { 73, "Snow",         "Neve"        },
    { 75, "Hvy snow",     "Neve forte"  },
    { 77, "Snow grains",  "Granizo neve"},
    { 80, "Showers",      "Pancadas"    },
    { 81, "Mod showers",  "Pancadas m"  },
    { 82, "Hvy showers",  "Pancadas f"  },
    { 85, "Snow shower",  "Neve panc"   },
    { 86, "Hvy sn showr", "Neve panc f" },
    { 95, "Thunderstorm", "Trovoada"    },
    { 96, "Tstm w hail",  "Trovoada g"  },
    { 99, "Tstm hail",    "Trovoada pg" },
};

static const uint8_t _wmoCount = sizeof(_wmoTable) / sizeof(_wmoTable[0]);

const char* weatherConditionName(uint8_t lang, uint8_t wmoCode) {
    // Exact match first
    for (uint8_t i = 0; i < _wmoCount; i++) {
        if (_wmoTable[i].code == wmoCode) {
            return (lang == LANG_PT) ? _wmoTable[i].pt : _wmoTable[i].en;
        }
    }
    // No exact match — fall back to "Clear"
    return (lang == LANG_PT) ? "Limpo" : "Clear";
}
