/*
 * test_data_fetcher_split.cpp
 *
 * Tests for the ticker-splitting logic in data_fetcher.cpp's
 * _splitTickers() function.  Because that function is static (private),
 * its algorithm is replicated here as a pure testable function.
 * Any change to _splitTickers() in data_fetcher.cpp must be mirrored here.
 */

#include "framework.h"
#include "config.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// ─── Algorithm replica from data_fetcher.cpp / _splitTickers() ───────────────
// Splits a comma-separated tickers string into an array of symbols.
// Trims leading/trailing whitespace from each token.
// Returns the number of symbols found (capped at QUOTES_MAX_TICKERS).

static uint8_t splitTickers(const char* input,
                             char symbols[][QUOTES_SYMBOL_MAX]) {
    char buf[QUOTES_TICKERS_MAX];
    strncpy(buf, input, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    uint8_t count = 0;
    char* saveptr = nullptr;
    char* tok = strtok_r(buf, ",", &saveptr);
    while (tok != nullptr && count < QUOTES_MAX_TICKERS) {
        while (*tok == ' ') tok++;
        char* end = tok + strlen(tok) - 1;
        while (end > tok && *end == ' ') { *end = '\0'; end--; }

        if (tok[0] != '\0') {
            strncpy(symbols[count], tok, QUOTES_SYMBOL_MAX - 1);
            symbols[count][QUOTES_SYMBOL_MAX - 1] = '\0';
            count++;
        }
        tok = strtok_r(nullptr, ",", &saveptr);
    }
    return count;
}

// ─── Basic splitting ──────────────────────────────────────────────────────────

SUITE("splitTickers — basic splitting") {
    char syms[QUOTES_MAX_TICKERS][QUOTES_SYMBOL_MAX];

    TEST("empty string returns 0 symbols") {
        ASSERT_EQ(splitTickers("", syms), 0);
    }

    TEST("single ticker returns 1 symbol") {
        uint8_t n = splitTickers("AAPL", syms);
        ASSERT_EQ(n, 1);
        ASSERT_STREQ(syms[0], "AAPL");
    }

    TEST("two tickers separated by comma") {
        uint8_t n = splitTickers("AAPL,MSFT", syms);
        ASSERT_EQ(n, 2);
        ASSERT_STREQ(syms[0], "AAPL");
        ASSERT_STREQ(syms[1], "MSFT");
    }

    TEST("three tickers") {
        uint8_t n = splitTickers("PETR4.SA,AAPL,BTC-USD", syms);
        ASSERT_EQ(n, 3);
        ASSERT_STREQ(syms[0], "PETR4.SA");
        ASSERT_STREQ(syms[1], "AAPL");
        ASSERT_STREQ(syms[2], "BTC-USD");
    }

    TEST("maximum of 8 tickers (QUOTES_MAX_TICKERS)") {
        uint8_t n = splitTickers("A,B,C,D,E,F,G,H,I,J", syms);
        ASSERT_EQ(n, (uint8_t)QUOTES_MAX_TICKERS);
    }

    TEST("9th ticker is silently dropped") {
        char input[] = "A,B,C,D,E,F,G,H,X";
        uint8_t n = splitTickers(input, syms);
        ASSERT_EQ(n, (uint8_t)QUOTES_MAX_TICKERS);
        // The 9th "X" must not appear in any slot
        bool found = false;
        for (uint8_t i = 0; i < n; i++) {
            if (strcmp(syms[i], "X") == 0) { found = true; break; }
        }
        ASSERT_FALSE(found);
    }
}

// ─── Whitespace trimming ──────────────────────────────────────────────────────

SUITE("splitTickers — whitespace trimming") {
    char syms[QUOTES_MAX_TICKERS][QUOTES_SYMBOL_MAX];

    TEST("leading space is trimmed") {
        uint8_t n = splitTickers(" AAPL", syms);
        ASSERT_EQ(n, 1);
        ASSERT_STREQ(syms[0], "AAPL");
    }

    TEST("trailing space is trimmed") {
        uint8_t n = splitTickers("AAPL ", syms);
        ASSERT_EQ(n, 1);
        ASSERT_STREQ(syms[0], "AAPL");
    }

    TEST("both leading and trailing spaces trimmed") {
        uint8_t n = splitTickers("  AAPL  ", syms);
        ASSERT_EQ(n, 1);
        ASSERT_STREQ(syms[0], "AAPL");
    }

    TEST("spaces around comma-separated tickers are trimmed") {
        uint8_t n = splitTickers("AAPL , MSFT , GOOG", syms);
        ASSERT_EQ(n, 3);
        ASSERT_STREQ(syms[0], "AAPL");
        ASSERT_STREQ(syms[1], "MSFT");
        ASSERT_STREQ(syms[2], "GOOG");
    }

    TEST("multiple leading spaces are all trimmed") {
        uint8_t n = splitTickers("   BTC-USD", syms);
        ASSERT_EQ(n, 1);
        ASSERT_STREQ(syms[0], "BTC-USD");
    }
}

// ─── Edge / empty tokens ──────────────────────────────────────────────────────

SUITE("splitTickers — empty tokens") {
    char syms[QUOTES_MAX_TICKERS][QUOTES_SYMBOL_MAX];

    TEST("leading comma (empty first token) is skipped") {
        uint8_t n = splitTickers(",AAPL", syms);
        ASSERT_EQ(n, 1);
        ASSERT_STREQ(syms[0], "AAPL");
    }

    TEST("trailing comma (empty last token) is skipped") {
        uint8_t n = splitTickers("AAPL,", syms);
        ASSERT_EQ(n, 1);
        ASSERT_STREQ(syms[0], "AAPL");
    }

    TEST("consecutive commas produce no extra entries") {
        uint8_t n = splitTickers("A,,B", syms);
        ASSERT_EQ(n, 2);
        ASSERT_STREQ(syms[0], "A");
        ASSERT_STREQ(syms[1], "B");
    }

    TEST("whitespace-only token between commas is skipped") {
        uint8_t n = splitTickers("A, ,B", syms);
        ASSERT_EQ(n, 2);
        ASSERT_STREQ(syms[0], "A");
        ASSERT_STREQ(syms[1], "B");
    }

    TEST("string of commas returns 0") {
        ASSERT_EQ(splitTickers(",,,", syms), 0);
    }
}

// ─── Symbol length capping ────────────────────────────────────────────────────

SUITE("splitTickers — symbol length safety") {
    char syms[QUOTES_MAX_TICKERS][QUOTES_SYMBOL_MAX];

    TEST("symbol is null-terminated within QUOTES_SYMBOL_MAX") {
        // A symbol longer than QUOTES_SYMBOL_MAX-1 must be truncated
        char long_sym[QUOTES_SYMBOL_MAX + 8];
        memset(long_sym, 'X', sizeof(long_sym) - 1);
        long_sym[sizeof(long_sym) - 1] = '\0';
        splitTickers(long_sym, syms);
        ASSERT_EQ(syms[0][QUOTES_SYMBOL_MAX - 1], '\0');
    }

    TEST("exact QUOTES_SYMBOL_MAX-1 chars fit without truncation") {
        // Build a symbol of exactly QUOTES_SYMBOL_MAX - 1 chars
        char sym[QUOTES_SYMBOL_MAX];
        memset(sym, 'A', QUOTES_SYMBOL_MAX - 1);
        sym[QUOTES_SYMBOL_MAX - 1] = '\0';
        splitTickers(sym, syms);
        ASSERT_EQ((int)strlen(syms[0]), QUOTES_SYMBOL_MAX - 1);
    }
}

// ─── Realistic inputs ─────────────────────────────────────────────────────────

SUITE("splitTickers — realistic ticker strings") {
    char syms[QUOTES_MAX_TICKERS][QUOTES_SYMBOL_MAX];

    TEST("Brazilian stocks with .SA suffix") {
        uint8_t n = splitTickers("PETR4.SA,VALE3.SA,ITUB4.SA", syms);
        ASSERT_EQ(n, 3);
        ASSERT_STREQ(syms[0], "PETR4.SA");
        ASSERT_STREQ(syms[1], "VALE3.SA");
        ASSERT_STREQ(syms[2], "ITUB4.SA");
    }

    TEST("mix of US stock, crypto and forex") {
        uint8_t n = splitTickers("AAPL, BTC-USD, BRL=X", syms);
        ASSERT_EQ(n, 3);
        ASSERT_STREQ(syms[0], "AAPL");
        ASSERT_STREQ(syms[1], "BTC-USD");
        ASSERT_STREQ(syms[2], "BRL=X");
    }
}
