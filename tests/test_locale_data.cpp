/*
 * test_locale_data.cpp
 *
 * Tests for localeDayName(), localeMonthName(), ianaToPostfix(),
 * tzTableSize(), tzTableEntry(), and weatherConditionName()
 * from locale_data.cpp.
 */

#include "framework.h"
#include "locale_data.h"
#include <string.h>

// ─── localeDayName ────────────────────────────────────────────────────────────

SUITE("localeDayName — English") {
    TEST("Sunday is SUN") {
        ASSERT_STREQ(localeDayName(LANG_EN, 0), "SUN");
    }
    TEST("Monday is MON") {
        ASSERT_STREQ(localeDayName(LANG_EN, 1), "MON");
    }
    TEST("Tuesday is TUE") {
        ASSERT_STREQ(localeDayName(LANG_EN, 2), "TUE");
    }
    TEST("Wednesday is WED") {
        ASSERT_STREQ(localeDayName(LANG_EN, 3), "WED");
    }
    TEST("Thursday is THU") {
        ASSERT_STREQ(localeDayName(LANG_EN, 4), "THU");
    }
    TEST("Friday is FRI") {
        ASSERT_STREQ(localeDayName(LANG_EN, 5), "FRI");
    }
    TEST("Saturday is SAT") {
        ASSERT_STREQ(localeDayName(LANG_EN, 6), "SAT");
    }
    TEST("index > 6 wraps to Sunday (DOM/SUN)") {
        // Documented: if dayIndex > 6, clamp to 0 (Sunday).
        ASSERT_STREQ(localeDayName(LANG_EN, 7), "SUN");
        ASSERT_STREQ(localeDayName(LANG_EN, 255), "SUN");
    }
}

SUITE("localeDayName — Portuguese") {
    TEST("Sunday is DOM") {
        ASSERT_STREQ(localeDayName(LANG_PT, 0), "DOM");
    }
    TEST("Monday is SEG") {
        ASSERT_STREQ(localeDayName(LANG_PT, 1), "SEG");
    }
    TEST("Tuesday is TER") {
        ASSERT_STREQ(localeDayName(LANG_PT, 2), "TER");
    }
    TEST("Wednesday is QUA") {
        ASSERT_STREQ(localeDayName(LANG_PT, 3), "QUA");
    }
    TEST("Thursday is QUI") {
        ASSERT_STREQ(localeDayName(LANG_PT, 4), "QUI");
    }
    TEST("Friday is SEX") {
        ASSERT_STREQ(localeDayName(LANG_PT, 5), "SEX");
    }
    TEST("Saturday is SAB") {
        ASSERT_STREQ(localeDayName(LANG_PT, 6), "SAB");
    }
    TEST("index > 6 wraps to Sunday") {
        ASSERT_STREQ(localeDayName(LANG_PT, 8), "DOM");
    }
}

SUITE("localeDayName — unknown language falls back to English") {
    TEST("unknown lang code falls back to English SUN") {
        ASSERT_STREQ(localeDayName(99, 0), "SUN");
    }
    TEST("unknown lang + Monday falls back to MON") {
        ASSERT_STREQ(localeDayName(200, 1), "MON");
    }
}

// ─── localeMonthName ──────────────────────────────────────────────────────────

SUITE("localeMonthName — English") {
    TEST("January is JAN") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 0), "JAN");
    }
    TEST("February is FEB") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 1), "FEB");
    }
    TEST("March is MAR") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 2), "MAR");
    }
    TEST("April is APR") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 3), "APR");
    }
    TEST("May is MAY") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 4), "MAY");
    }
    TEST("June is JUN") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 5), "JUN");
    }
    TEST("July is JUL") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 6), "JUL");
    }
    TEST("August is AUG") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 7), "AUG");
    }
    TEST("September is SEP") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 8), "SEP");
    }
    TEST("October is OCT") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 9), "OCT");
    }
    TEST("November is NOV") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 10), "NOV");
    }
    TEST("December is DEC") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 11), "DEC");
    }
    TEST("index > 11 wraps to January") {
        ASSERT_STREQ(localeMonthName(LANG_EN, 12), "JAN");
        ASSERT_STREQ(localeMonthName(LANG_EN, 255), "JAN");
    }
}

SUITE("localeMonthName — Portuguese") {
    TEST("January is JAN") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 0), "JAN");
    }
    TEST("February is FEV") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 1), "FEV");
    }
    TEST("April is ABR") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 3), "ABR");
    }
    TEST("May is MAI") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 4), "MAI");
    }
    TEST("August is AGO") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 7), "AGO");
    }
    TEST("September is SET") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 8), "SET");
    }
    TEST("October is OUT") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 9), "OUT");
    }
    TEST("December is DEZ") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 11), "DEZ");
    }
    TEST("index > 11 wraps to January") {
        ASSERT_STREQ(localeMonthName(LANG_PT, 13), "JAN");
    }
}

// ─── ianaToPostfix ────────────────────────────────────────────────────────────

SUITE("ianaToPostfix — known zones") {
    TEST("UTC maps to UTC0") {
        ASSERT_STREQ(ianaToPostfix("UTC"), "UTC0");
    }
    TEST("America/Sao_Paulo returns a non-null POSIX string") {
        const char* tz = ianaToPostfix("America/Sao_Paulo");
        ASSERT_TRUE(tz != nullptr);
        ASSERT_TRUE(strlen(tz) > 0);
    }
    TEST("America/Sao_Paulo POSIX string starts with BRT") {
        const char* tz = ianaToPostfix("America/Sao_Paulo");
        ASSERT_TRUE(strncmp(tz, "BRT", 3) == 0);
    }
    TEST("America/Fortaleza is BRT3 (no DST)") {
        ASSERT_STREQ(ianaToPostfix("America/Fortaleza"), "BRT3");
    }
    TEST("America/New_York contains EST5EDT") {
        const char* tz = ianaToPostfix("America/New_York");
        ASSERT_TRUE(tz != nullptr);
        ASSERT_TRUE(strstr(tz, "EST5EDT") != nullptr);
    }
    TEST("America/Los_Angeles contains PST8PDT") {
        const char* tz = ianaToPostfix("America/Los_Angeles");
        ASSERT_TRUE(tz != nullptr);
        ASSERT_TRUE(strstr(tz, "PST8PDT") != nullptr);
    }
    TEST("Europe/Paris contains CET-1") {
        const char* tz = ianaToPostfix("Europe/Paris");
        ASSERT_TRUE(tz != nullptr);
        ASSERT_TRUE(strstr(tz, "CET") != nullptr);
    }
    TEST("Asia/Tokyo is JST-9") {
        ASSERT_STREQ(ianaToPostfix("Asia/Tokyo"), "JST-9");
    }
    TEST("Asia/Kolkata contains IST-5:30") {
        ASSERT_STREQ(ianaToPostfix("Asia/Kolkata"), "IST-5:30");
    }
    TEST("America/Manaus is AMT4") {
        ASSERT_STREQ(ianaToPostfix("America/Manaus"), "AMT4");
    }
}

SUITE("ianaToPostfix — unknown / edge cases") {
    TEST("unknown zone returns nullptr") {
        ASSERT_TRUE(ianaToPostfix("Mars/Olympus") == nullptr);
    }
    TEST("empty string returns nullptr") {
        ASSERT_TRUE(ianaToPostfix("") == nullptr);
    }
    TEST("null returns nullptr") {
        ASSERT_TRUE(ianaToPostfix(nullptr) == nullptr);
    }
    TEST("partial match is not accepted") {
        // "America/Sao" is not in the table — must be exact
        ASSERT_TRUE(ianaToPostfix("America/Sao") == nullptr);
    }
    TEST("case matters — lower-case fails") {
        ASSERT_TRUE(ianaToPostfix("utc") == nullptr);
    }
}

// ─── tzTableSize / tzTableEntry ───────────────────────────────────────────────

SUITE("tzTableSize / tzTableEntry") {
    TEST("table has at least 10 entries") {
        ASSERT_TRUE(tzTableSize() >= 10);
    }
    TEST("entry 0 is non-null") {
        ASSERT_TRUE(tzTableEntry(0) != nullptr);
    }
    TEST("entry 0 has non-empty IANA name") {
        ASSERT_TRUE(strlen(tzTableEntry(0)->iana) > 0);
    }
    TEST("entry 0 has non-empty POSIX string") {
        ASSERT_TRUE(strlen(tzTableEntry(0)->posix) > 0);
    }
    TEST("out-of-bounds index returns nullptr") {
        ASSERT_TRUE(tzTableEntry(tzTableSize()) == nullptr);
        ASSERT_TRUE(tzTableEntry(255) == nullptr);
    }
    TEST("all entries have non-empty iana and posix") {
        uint8_t n = tzTableSize();
        bool allOk = true;
        for (uint8_t i = 0; i < n; i++) {
            const TZEntry* e = tzTableEntry(i);
            if (!e || strlen(e->iana) == 0 || strlen(e->posix) == 0) {
                allOk = false;
                break;
            }
        }
        ASSERT_TRUE(allOk);
    }
    TEST("every iana name is unique") {
        uint8_t n = tzTableSize();
        bool unique = true;
        for (uint8_t i = 0; i < n && unique; i++) {
            for (uint8_t j = i + 1; j < n && unique; j++) {
                if (strcmp(tzTableEntry(i)->iana, tzTableEntry(j)->iana) == 0)
                    unique = false;
            }
        }
        ASSERT_TRUE(unique);
    }
    TEST("ianaToPostfix result matches tzTableEntry for same name") {
        uint8_t n = tzTableSize();
        bool match = true;
        for (uint8_t i = 0; i < n && match; i++) {
            const TZEntry* e = tzTableEntry(i);
            if (strcmp(ianaToPostfix(e->iana), e->posix) != 0) match = false;
        }
        ASSERT_TRUE(match);
    }
}

// ─── weatherConditionName ─────────────────────────────────────────────────────

SUITE("weatherConditionName — English") {
    TEST("code 0 (clear) is Clear") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 0), "Clear");
    }
    TEST("code 2 (partly cloudy) is Part cloudy") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 2), "Part cloudy");
    }
    TEST("code 45 (fog) is Fog") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 45), "Fog");
    }
    TEST("code 61 (light rain) is Lt rain") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 61), "Lt rain");
    }
    TEST("code 65 (heavy rain) is Hvy rain") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 65), "Hvy rain");
    }
    TEST("code 71 (light snow) is Lt snow") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 71), "Lt snow");
    }
    TEST("code 95 (thunderstorm) is Thunderstorm") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 95), "Thunderstorm");
    }
    TEST("code 99 (thunderstorm + hail) is Tstm hail") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 99), "Tstm hail");
    }
    TEST("unknown code (e.g. 100) falls back to Clear") {
        ASSERT_STREQ(weatherConditionName(LANG_EN, 100), "Clear");
    }
}

SUITE("weatherConditionName — Portuguese") {
    TEST("code 0 (clear) is Limpo") {
        ASSERT_STREQ(weatherConditionName(LANG_PT, 0), "Limpo");
    }
    TEST("code 2 (partly cloudy) is Nublado") {
        ASSERT_STREQ(weatherConditionName(LANG_PT, 2), "Nublado");
    }
    TEST("code 45 (fog) is Nevoa") {
        ASSERT_STREQ(weatherConditionName(LANG_PT, 45), "Nevoa");
    }
    TEST("code 63 (rain) is Chuva") {
        ASSERT_STREQ(weatherConditionName(LANG_PT, 63), "Chuva");
    }
    TEST("code 80 (showers) is Pancadas") {
        ASSERT_STREQ(weatherConditionName(LANG_PT, 80), "Pancadas");
    }
    TEST("code 95 (thunderstorm) is Trovoada") {
        ASSERT_STREQ(weatherConditionName(LANG_PT, 95), "Trovoada");
    }
    TEST("unknown code falls back to Limpo") {
        ASSERT_STREQ(weatherConditionName(LANG_PT, 200), "Limpo");
    }
}

SUITE("weatherConditionName — unknown language falls back to English") {
    TEST("unknown lang returns English string for code 0") {
        ASSERT_STREQ(weatherConditionName(99, 0), "Clear");
    }
    TEST("unknown lang returns English fallback for unknown code") {
        ASSERT_STREQ(weatherConditionName(99, 250), "Clear");
    }
}
