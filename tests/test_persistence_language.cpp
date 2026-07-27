/*
 * test_persistence_language.cpp
 *
 * Tests for:
 *  - isUiLanguageValid() from persistence.cpp
 *  - formatQuotePrice() from text_encoding.cpp
 *  - Config-constant sanity checks (compile-time values in config.h/globals.h)
 */

#include "framework.h"
#include "text_encoding.h"
#include "config.h"

// ─── isUiLanguageValid ────────────────────────────────────────────────────────
// The function lives in persistence.cpp but persistence.cpp pulls in
// Preferences.h (Arduino).  Instead of linking the full module, we copy its
// logic here (it is trivially short) so this test file stays Arduino-free.
// If the accepted language list in persistence.cpp ever changes, update this
// stub accordingly.

static const char* const _uiLangs[] = { "en", "pt" };
static const int _uiLangCount = 2;

static bool isUiLanguageValid(const char* code) {
    if (!code) return false;
    for (int i = 0; i < _uiLangCount; i++) {
        if (strcmp(code, _uiLangs[i]) == 0) return true;
    }
    return false;
}

SUITE("isUiLanguageValid — accepted codes") {
    TEST("\"en\" is valid") {
        ASSERT_TRUE(isUiLanguageValid("en"));
    }
    TEST("\"pt\" is valid") {
        ASSERT_TRUE(isUiLanguageValid("pt"));
    }
}

SUITE("isUiLanguageValid — rejected codes") {
    TEST("\"EN\" (uppercase) is invalid") {
        ASSERT_FALSE(isUiLanguageValid("EN"));
    }
    TEST("\"PT\" (uppercase) is invalid") {
        ASSERT_FALSE(isUiLanguageValid("PT"));
    }
    TEST("\"fr\" is invalid") {
        ASSERT_FALSE(isUiLanguageValid("fr"));
    }
    TEST("\"de\" is invalid") {
        ASSERT_FALSE(isUiLanguageValid("de"));
    }
    TEST("empty string is invalid") {
        ASSERT_FALSE(isUiLanguageValid(""));
    }
    TEST("null is invalid") {
        ASSERT_FALSE(isUiLanguageValid(nullptr));
    }
    TEST("space is invalid") {
        ASSERT_FALSE(isUiLanguageValid(" "));
    }
    TEST("\"e\" (partial) is invalid") {
        ASSERT_FALSE(isUiLanguageValid("e"));
    }
}

// ─── formatQuotePrice ─────────────────────────────────────────────────────────

SUITE("formatQuotePrice — English locale (. thousands, , decimal)") {
    char dst[32];

    TEST("value < 1000 has no thousands separator") {
        formatQuotePrice(38.42f, "en", dst, sizeof(dst));
        ASSERT_STREQ(dst, "38.42");
    }

    TEST("value 1234.5 -> 1,234.50") {
        formatQuotePrice(1234.5f, "en", dst, sizeof(dst));
        ASSERT_STREQ(dst, "1,234.50");
    }

    TEST("value 12345.0 -> 12,345.00") {
        formatQuotePrice(12345.0f, "en", dst, sizeof(dst));
        ASSERT_STREQ(dst, "12,345.00");
    }

    TEST("value 1234567.0 -> 1,234,567.00") {
        formatQuotePrice(1234567.0f, "en", dst, sizeof(dst));
        ASSERT_STREQ(dst, "1,234,567.00");
    }

    TEST("zero -> 0.00") {
        formatQuotePrice(0.0f, "en", dst, sizeof(dst));
        ASSERT_STREQ(dst, "0.00");
    }

    TEST("one cent -> 0.01") {
        formatQuotePrice(0.01f, "en", dst, sizeof(dst));
        ASSERT_STREQ(dst, "0.01");
    }

    TEST("negative value -> -38.42") {
        formatQuotePrice(-38.42f, "en", dst, sizeof(dst));
        ASSERT_STREQ(dst, "-38.42");
    }

    TEST("negative thousands -> -1,234.56") {
        formatQuotePrice(-1234.56f, "en", dst, sizeof(dst));
        ASSERT_STREQ(dst, "-1,234.56");
    }

    TEST("null locale falls back to english (. decimal)") {
        formatQuotePrice(9.99f, nullptr, dst, sizeof(dst));
        ASSERT_STREQ(dst, "9.99");
    }

    TEST("unknown locale (e.g. \"fr\") falls back to english") {
        formatQuotePrice(1234.0f, "fr", dst, sizeof(dst));
        ASSERT_STREQ(dst, "1,234.00");
    }
}

SUITE("formatQuotePrice — Portuguese locale (, thousands, . decimal) ") {
    char dst[32];

    TEST("value < 1000 has no separator") {
        formatQuotePrice(38.42f, "pt", dst, sizeof(dst));
        ASSERT_STREQ(dst, "38,42");
    }

    TEST("value 1234.5 -> 1.234,50") {
        formatQuotePrice(1234.5f, "pt", dst, sizeof(dst));
        ASSERT_STREQ(dst, "1.234,50");
    }

    TEST("value 12345.0 -> 12.345,00") {
        formatQuotePrice(12345.0f, "pt", dst, sizeof(dst));
        ASSERT_STREQ(dst, "12.345,00");
    }

    TEST("value 1234567.0 -> 1.234.567,00") {
        formatQuotePrice(1234567.0f, "pt", dst, sizeof(dst));
        ASSERT_STREQ(dst, "1.234.567,00");
    }

    TEST("zero -> 0,00") {
        formatQuotePrice(0.0f, "pt", dst, sizeof(dst));
        ASSERT_STREQ(dst, "0,00");
    }

    TEST("negative value -> -38,42") {
        formatQuotePrice(-38.42f, "pt", dst, sizeof(dst));
        ASSERT_STREQ(dst, "-38,42");
    }

    TEST("negative thousands -> -1.234,56") {
        formatQuotePrice(-1234.56f, "pt", dst, sizeof(dst));
        ASSERT_STREQ(dst, "-1.234,56");
    }

    TEST("'PT' uppercase also triggers pt locale") {
        formatQuotePrice(1234.5f, "PT", dst, sizeof(dst));
        ASSERT_STREQ(dst, "1.234,50");
    }
}

SUITE("formatQuotePrice — buffer boundary") {
    char dst[8];

    TEST("output is always null-terminated within maxLen") {
        formatQuotePrice(1234567.89f, "en", dst, sizeof(dst));
        ASSERT_EQ(dst[7], '\0');
    }

    TEST("maxLen=1 produces empty string") {
        formatQuotePrice(99.0f, "en", dst, 1);
        ASSERT_STREQ(dst, "");
    }

    TEST("maxLen=5 truncates to 4 chars") {
        formatQuotePrice(1.23f, "en", dst, 5);
        ASSERT_EQ((int)strlen(dst), 4);   // "1.23" exactly
        ASSERT_STREQ(dst, "1.23");
    }
}

// ─── config.h constant sanity checks ─────────────────────────────────────────
// These "tests" are compile-time constant verifications expressed as runtime
// assertions.  They catch accidental changes to defaults that would break the
// firmware contract.

SUITE("config constants — compile-time sanity") {
    TEST("DEFAULT_BRIGHTNESS is 0-15") {
        ASSERT_TRUE(DEFAULT_BRIGHTNESS >= 0 && DEFAULT_BRIGHTNESS <= 15);
    }
    TEST("DEFAULT_SCROLL_SPEED_MS is 10-200") {
        ASSERT_TRUE(DEFAULT_SCROLL_SPEED_MS >= 10 && DEFAULT_SCROLL_SPEED_MS <= 200);
    }
    TEST("MAX_MESSAGE_LEN > 0") {
        ASSERT_TRUE(MAX_MESSAGE_LEN > 0);
    }
    TEST("SCROLL_BUF_LEN >= MAX_MESSAGE_LEN") {
        ASSERT_TRUE(SCROLL_BUF_LEN >= MAX_MESSAGE_LEN);
    }
    TEST("NTP_RESYNC_MS is at least 1 minute") {
        ASSERT_TRUE(NTP_RESYNC_MS >= 60000UL);
    }
    TEST("MESSAGE_HISTORY_SIZE > 0") {
        ASSERT_TRUE(MESSAGE_HISTORY_SIZE > 0);
    }
    TEST("BLINK_INTERVAL_MS is positive") {
        ASSERT_TRUE(BLINK_INTERVAL_MS > 0);
    }
    TEST("CLOCK_MODE_HHMM == 0") {
        ASSERT_EQ(CLOCK_MODE_HHMM, 0);
    }
    TEST("CLOCK_MODE_HHMMSS == 1") {
        ASSERT_EQ(CLOCK_MODE_HHMMSS, 1);
    }
    TEST("CLOCK_MODE_DEFAULT == CLOCK_MODE_HHMM") {
        ASSERT_EQ(CLOCK_MODE_DEFAULT, CLOCK_MODE_HHMM);
    }
    TEST("QUOTES_MAX_TICKERS > 0 and fits in uint8_t") {
        ASSERT_TRUE(QUOTES_MAX_TICKERS > 0 && QUOTES_MAX_TICKERS <= 255);
    }
    TEST("QUOTES_SYMBOL_MAX is positive") {
        ASSERT_TRUE(QUOTES_SYMBOL_MAX > 0);
    }
    TEST("SLOT_SCHEDULE_MAX_MINUTE == 1439") {
        ASSERT_EQ(SLOT_SCHEDULE_MAX_MINUTE, 1439);
    }
    TEST("SLOT_SCHEDULE_ALL_DAYS == 0x7F") {
        ASSERT_EQ(SLOT_SCHEDULE_ALL_DAYS, 0x7F);
    }
    TEST("DEFAULT_NIGHT_START_MIN is valid minute (0-1439)") {
        ASSERT_TRUE(DEFAULT_NIGHT_START_MIN <= 1439);
    }
    TEST("DEFAULT_NIGHT_END_MIN is valid minute (0-1439)") {
        ASSERT_TRUE(DEFAULT_NIGHT_END_MIN <= 1439);
    }
    TEST("QUOTES_SCHED_START_DEFAULT_MIN < QUOTES_SCHED_END_DEFAULT_MIN") {
        // Business hours: start < end (no midnight crossing)
        ASSERT_TRUE(QUOTES_SCHED_START_DEFAULT_MIN < QUOTES_SCHED_END_DEFAULT_MIN);
    }
    TEST("UI_LANG_CODE_MAX fits en and pt with null terminator") {
        ASSERT_TRUE(UI_LANG_CODE_MAX >= 3);
    }
    TEST("LOCALE_CODE_MAX fits en and pt with null terminator") {
        ASSERT_TRUE(LOCALE_CODE_MAX >= 3);
    }
    TEST("NVS_NAMESPACE is non-empty") {
        ASSERT_TRUE(strlen(NVS_NAMESPACE) > 0);
    }
    TEST("all NVS keys fit in NVS 15-char key limit") {
        // ESP32 NVS imposes a 15-character key limit.
        struct { const char* key; } keys[] = {
            { NVS_KEY_BRIGHTNESS }, { NVS_KEY_SCROLL_SPD },
            { NVS_KEY_TIMEZONE   }, { NVS_KEY_LOCALE     },
            { NVS_KEY_NTP_SERVER }, { NVS_KEY_WIFI_SSID  },
            { NVS_KEY_WIFI_PASS  }, { NVS_KEY_SLOT0_EN   },
            { NVS_KEY_SLOT1_EN   }, { NVS_KEY_SLOT2_EN   },
            { NVS_KEY_SLOT3_EN   }, { NVS_KEY_SLOT2_MS   },
            { NVS_KEY_SLOT3_MS   }, { NVS_KEY_DATE_INT_MS},
            { NVS_KEY_DATE_EN    }, { NVS_KEY_MESSAGE_MODE},
            { NVS_KEY_MESSAGE_DUR}, { NVS_KEY_UI_LANGUAGE},
            { NVS_KEY_CLOCK_MODE },
            { NVS_KEY_WEATHER_EN }, { NVS_KEY_WEATHER_UPMS},
            { NVS_KEY_WEATHER_LAT}, { NVS_KEY_WEATHER_LON },
            { NVS_KEY_TEMP_UNIT  },
            { NVS_KEY_QUOTES_EN  }, { NVS_KEY_QUOTES_UPMS },
            { NVS_KEY_QUOTES_TICKERS },
            { NVS_KEY_SLOT2_SCHED_START }, { NVS_KEY_SLOT2_SCHED_END },
            { NVS_KEY_SLOT2_SCHED_DAYS  }, { NVS_KEY_SLOT3_SCHED_START },
            { NVS_KEY_SLOT3_SCHED_END   }, { NVS_KEY_SLOT3_SCHED_DAYS  },
            { NVS_KEY_NIGHT_BRI_EN  }, { NVS_KEY_NIGHT_BRI_LVL },
            { NVS_KEY_NIGHT_START   }, { NVS_KEY_NIGHT_END     },
        };
        bool allFit = true;
        for (auto& k : keys) {
            if (strlen(k.key) > 15) { allFit = false; break; }
        }
        ASSERT_TRUE(allFit);
    }
}
