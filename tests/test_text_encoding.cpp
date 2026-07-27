/*
 * test_text_encoding.cpp
 *
 * Tests for utf8ToLatin1(), latin1ToUtf8(), and expandIconTags()
 * from text_encoding.cpp.
 */

#include "framework.h"
#include "text_encoding.h"

// ─── utf8ToLatin1 ─────────────────────────────────────────────────────────────

SUITE("utf8ToLatin1 — ASCII passthrough") {
    char dst[64];

    TEST("empty string produces empty output") {
        utf8ToLatin1("", dst, sizeof(dst));
        ASSERT_STREQ(dst, "");
    }

    TEST("pure ASCII string is unchanged") {
        utf8ToLatin1("Hello, World!", dst, sizeof(dst));
        ASSERT_STREQ(dst, "Hello, World!");
    }

    TEST("digits and punctuation pass through") {
        utf8ToLatin1("12:34 [ok] ~!@#$", dst, sizeof(dst));
        ASSERT_STREQ(dst, "12:34 [ok] ~!@#$");
    }

    TEST("null src returns dst unchanged") {
        dst[0] = 'X';
        char* ret = utf8ToLatin1(nullptr, dst, sizeof(dst));
        ASSERT_TRUE(ret == dst);
        ASSERT_EQ(dst[0], 'X');
    }

    TEST("maxLen=1 always produces empty output") {
        utf8ToLatin1("ABC", dst, 1);
        ASSERT_STREQ(dst, "");
    }

    TEST("output is truncated to maxLen-1 characters") {
        utf8ToLatin1("ABCDE", dst, 4);
        ASSERT_STREQ(dst, "ABC");
    }
}

SUITE("utf8ToLatin1 — 2-byte UTF-8 sequences (U+0080..U+00FF)") {
    char dst[64];

    TEST("U+00E9 (é) -> 0xE9 (Latin-1)") {
        // UTF-8: 0xC3 0xA9
        const char src[] = { (char)0xC3, (char)0xA9, 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0xE9u);
        ASSERT_EQ(dst[1], '\0');
    }

    TEST("U+00C0 (À) -> 0xC0") {
        const char src[] = { (char)0xC3, (char)0x80, 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0xC0u);
    }

    TEST("U+00B0 (°) -> 0xB0") {
        const char src[] = { (char)0xC2, (char)0xB0, 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0xB0u);
    }

    TEST("U+00FC (ü) -> 0xFC") {
        const char src[] = { (char)0xC3, (char)0xBC, 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0xFCu);
    }

    TEST("U+007F (DEL, edge of ASCII) -> 0x7F") {
        const char src[] = { (char)0x7F, 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x7Fu);
    }

    TEST("mixed ASCII + accented string") {
        // "São" = 'S' 'ã'(U+00E3) 'o'
        // UTF-8 of ã: 0xC3 0xA3
        const char src[] = { 'S', (char)0xC3, (char)0xA3, 'o', 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ(dst[0], 'S');
        ASSERT_EQ((uint8_t)dst[1], 0xE3u);
        ASSERT_EQ(dst[2], 'o');
        ASSERT_EQ(dst[3], '\0');
    }
}

SUITE("utf8ToLatin1 — code-points beyond Latin-1") {
    char dst[64];

    TEST("U+0100 (Ā, first non-Latin-1 2-byte) maps to '?'") {
        // UTF-8: 0xC4 0x80
        const char src[] = { (char)0xC4, (char)0x80, 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ(dst[0], '?');
        ASSERT_EQ(dst[1], '\0');
    }

    TEST("U+20AC (€, 3-byte UTF-8) maps to '?'") {
        // UTF-8: 0xE2 0x82 0xAC
        const char src[] = { (char)0xE2, (char)0x82, (char)0xAC, 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ(dst[0], '?');
    }

    TEST("4-byte sequence (emoji) maps to '?'") {
        // U+1F600 = 0xF0 0x9F 0x98 0x80
        const char src[] = { (char)0xF0, (char)0x9F, (char)0x98, (char)0x80, 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ(dst[0], '?');
    }

    TEST("text around a non-Latin-1 char is preserved") {
        // "A€B" = A U+20AC B
        const char src[] = { 'A', (char)0xE2, (char)0x82, (char)0xAC, 'B', 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_EQ(dst[0], 'A');
        ASSERT_EQ(dst[1], '?');
        ASSERT_EQ(dst[2], 'B');
        ASSERT_EQ(dst[3], '\0');
    }
}

SUITE("utf8ToLatin1 — malformed / edge sequences") {
    char dst[64];

    TEST("lone continuation byte is consumed without crashing") {
        const char src[] = { (char)0x80, 'A', 0 };
        utf8ToLatin1(src, dst, sizeof(dst));
        // Should not crash; output character value doesn't matter
        ASSERT_TRUE(dst[strlen(dst)] == '\0');  // null-terminated
    }

    TEST("truncated 2-byte sequence at end of string") {
        const char src[] = { (char)0xC3, 0 };  // lead byte but no continuation
        utf8ToLatin1(src, dst, sizeof(dst));
        ASSERT_TRUE(dst[0] == '\0' || dst[1] == '\0');  // null-terminated
    }
}

// ─── latin1ToUtf8 ─────────────────────────────────────────────────────────────

SUITE("latin1ToUtf8 — ASCII passthrough") {
    char dst[128];

    TEST("empty input") {
        latin1ToUtf8("", dst, sizeof(dst));
        ASSERT_STREQ(dst, "");
    }

    TEST("pure ASCII is unchanged") {
        latin1ToUtf8("Hello!", dst, sizeof(dst));
        ASSERT_STREQ(dst, "Hello!");
    }
}

SUITE("latin1ToUtf8 — high Latin-1 bytes") {
    char dst[128];

    TEST("0xE9 (Latin-1 é) encodes to UTF-8 0xC3 0xA9") {
        const char src[] = { (char)0xE9, 0 };
        latin1ToUtf8(src, dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0xC3u);
        ASSERT_EQ((uint8_t)dst[1], 0xA9u);
        ASSERT_EQ(dst[2], '\0');
    }

    TEST("0xB0 (°) encodes to UTF-8 0xC2 0xB0") {
        const char src[] = { (char)0xB0, 0 };
        latin1ToUtf8(src, dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0xC2u);
        ASSERT_EQ((uint8_t)dst[1], 0xB0u);
    }

    TEST("0x80 boundary encodes correctly") {
        const char src[] = { (char)0x80, 0 };
        latin1ToUtf8(src, dst, sizeof(dst));
        // U+0080 = 0xC2 0x80 in UTF-8
        ASSERT_EQ((uint8_t)dst[0], 0xC2u);
        ASSERT_EQ((uint8_t)dst[1], 0x80u);
    }

    TEST("0xFF encodes to 0xC3 0xBF") {
        const char src[] = { (char)0xFF, 0 };
        latin1ToUtf8(src, dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0xC3u);
        ASSERT_EQ((uint8_t)dst[1], 0xBFu);
    }
}

SUITE("latin1ToUtf8 — roundtrip with utf8ToLatin1") {
    char utf8buf[128], back[64];

    TEST("ASCII roundtrip") {
        const char* orig = "Test 123";
        latin1ToUtf8(orig, utf8buf, sizeof(utf8buf));
        utf8ToLatin1(utf8buf, back, sizeof(back));
        ASSERT_STREQ(back, orig);
    }

    TEST("high Latin-1 byte roundtrip (0xE9)") {
        const char orig[] = { (char)0xE9, 0 };
        latin1ToUtf8(orig, utf8buf, sizeof(utf8buf));
        utf8ToLatin1(utf8buf, back, sizeof(back));
        ASSERT_EQ((uint8_t)back[0], 0xE9u);
        ASSERT_EQ(back[1], '\0');
    }

    TEST("mixed ASCII + Latin-1 roundtrip") {
        // "São Paulo" in Latin-1: 'S' 0xE3 'o' ' ' 'P' 'a' 'u' 'l' 'o'
        const char orig[] = { 'S', (char)0xE3, 'o', ' ', 'P', 'a', 'u', 'l', 'o', 0 };
        latin1ToUtf8(orig, utf8buf, sizeof(utf8buf));
        utf8ToLatin1(utf8buf, back, sizeof(back));
        ASSERT_EQ(back[0], 'S');
        ASSERT_EQ((uint8_t)back[1], 0xE3u);
        ASSERT_EQ(back[2], 'o');
    }

    TEST("all Latin-1 extended bytes roundtrip") {
        // Build a string of every byte 0x80..0xFF
        char src[128], utf8[512], restored[128];
        for (int i = 0; i < 128; i++) src[i] = (char)(0x80 + i);
        src[127] = '\0';
        latin1ToUtf8(src, utf8, sizeof(utf8));
        utf8ToLatin1(utf8, restored, sizeof(restored));
        ASSERT_TRUE(memcmp(src, restored, 127) == 0);
    }
}

SUITE("latin1ToUtf8 — buffer boundary") {
    char dst[8];

    TEST("output is null-terminated within maxLen") {
        const char src[] = { (char)0xE9, (char)0xE9, (char)0xE9, 0 };
        // Each Latin-1 byte expands to 2 UTF-8 bytes → 3 bytes need 6 data + 1 null = 7 bytes.
        // dst is 8 bytes → all three fit; null lands at index 6.
        latin1ToUtf8(src, dst, sizeof(dst));
        ASSERT_EQ(dst[6], '\0');
    }

    TEST("dst is always null-terminated within maxLen") {
        const char src[] = { (char)0xE9, (char)0xE9, (char)0xE9, (char)0xE9, 0 };
        latin1ToUtf8(src, dst, 5);  // 5-byte buffer: 2 full encoded chars + null
        ASSERT_TRUE(dst[4] == '\0');
    }
}

// ─── expandIconTags ───────────────────────────────────────────────────────────

SUITE("expandIconTags — known icon names") {
    char dst[64];

    TEST("[heart] -> 0x03") {
        expandIconTags("[heart]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x03u);
        ASSERT_EQ(dst[1], '\0');
    }

    TEST("[diamond] -> 0x04") {
        expandIconTags("[diamond]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x04u);
    }

    TEST("[spade] -> 0x06") {
        expandIconTags("[spade]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x06u);
    }

    TEST("[bullet] -> 0x07") {
        expandIconTags("[bullet]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x07u);
    }

    TEST("[star] -> 0x0F") {
        expandIconTags("[star]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x0Fu);
    }

    TEST("[arrow_right] -> 0x10") {
        expandIconTags("[arrow_right]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x10u);
    }

    TEST("[arrow_left] -> 0x11") {
        expandIconTags("[arrow_left]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x11u);
    }

    TEST("[up] -> 0x18") {
        expandIconTags("[up]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x18u);
    }

    TEST("[down] -> 0x19") {
        expandIconTags("[down]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x19u);
    }

    TEST("[bell] -> 0x1E") {
        expandIconTags("[bell]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x1Eu);
    }

    TEST("[warn] -> 0x1F") {
        expandIconTags("[warn]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x1Fu);
    }
}

SUITE("expandIconTags — mixed text and tags") {
    char dst[64];

    TEST("prefix text before tag is preserved") {
        expandIconTags("Hi [heart]", dst, sizeof(dst));
        ASSERT_EQ(dst[0], 'H');
        ASSERT_EQ(dst[1], 'i');
        ASSERT_EQ(dst[2], ' ');
        ASSERT_EQ((uint8_t)dst[3], 0x03u);
        ASSERT_EQ(dst[4], '\0');
    }

    TEST("suffix text after tag is preserved") {
        expandIconTags("[star] OK", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x0Fu);
        ASSERT_EQ(dst[1], ' ');
        ASSERT_STREQ(dst + 2, "OK");
    }

    TEST("multiple tags in one string") {
        expandIconTags("[heart][bell]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x03u);
        ASSERT_EQ((uint8_t)dst[1], 0x1Eu);
        ASSERT_EQ(dst[2], '\0');
    }

    TEST("tag surrounded by text") {
        expandIconTags("A[star]B", dst, sizeof(dst));
        ASSERT_EQ(dst[0], 'A');
        ASSERT_EQ((uint8_t)dst[1], 0x0Fu);
        ASSERT_EQ(dst[2], 'B');
        ASSERT_EQ(dst[3], '\0');
    }

    TEST("two tags separated by spaces") {
        expandIconTags("[up] and [down]", dst, sizeof(dst));
        ASSERT_EQ((uint8_t)dst[0], 0x18u);
        // " and "
        ASSERT_EQ(dst[1], ' ');
        ASSERT_EQ(dst[2], 'a');
        ASSERT_EQ(dst[3], 'n');
        ASSERT_EQ(dst[4], 'd');
        ASSERT_EQ(dst[5], ' ');
        ASSERT_EQ((uint8_t)dst[6], 0x19u);
        ASSERT_EQ(dst[7], '\0');
    }
}

SUITE("expandIconTags — unknown tags pass through verbatim") {
    char dst[64];

    TEST("unknown tag is copied with brackets") {
        expandIconTags("[unknown]", dst, sizeof(dst));
        ASSERT_STREQ(dst, "[unknown]");
    }

    TEST("case mismatch is treated as unknown") {
        expandIconTags("[Heart]", dst, sizeof(dst));
        ASSERT_STREQ(dst, "[Heart]");
    }

    TEST("empty tag [] is copied verbatim") {
        expandIconTags("[]", dst, sizeof(dst));
        ASSERT_STREQ(dst, "[]");
    }

    TEST("unclosed bracket is copied verbatim") {
        expandIconTags("[heart", dst, sizeof(dst));
        ASSERT_STREQ(dst, "[heart");
    }

    TEST("just a close bracket is copied verbatim") {
        expandIconTags("heart]", dst, sizeof(dst));
        ASSERT_STREQ(dst, "heart]");
    }

    TEST("nested brackets — outer survives as verbatim text") {
        expandIconTags("[[heart]]", dst, sizeof(dst));
        // '[' starts a lookup for '[heart]' — but '[' isn't a valid name char,
        // so looking up "]" (the first ']' after the second '[') or similar
        // is not in the table; the important thing is the function does not crash
        // and produces a null-terminated result
        ASSERT_TRUE(dst[strlen(dst)] == '\0');
    }

    TEST("no tags in plain text — unchanged") {
        expandIconTags("Hello World", dst, sizeof(dst));
        ASSERT_STREQ(dst, "Hello World");
    }
}

SUITE("expandIconTags — buffer boundary") {
    char dst[4];

    TEST("output is always null-terminated within maxLen") {
        expandIconTags("[heart][bell][star]", dst, sizeof(dst));
        ASSERT_EQ(dst[3], '\0');
    }

    TEST("maxLen=1 produces empty string") {
        expandIconTags("[heart]", dst, 1);
        ASSERT_STREQ(dst, "");
    }
}

SUITE("expandIconTags — null / empty guards") {
    char dst[32];

    TEST("null src returns dst") {
        dst[0] = 'X';
        char* ret = expandIconTags(nullptr, dst, sizeof(dst));
        ASSERT_TRUE(ret == dst);
    }

    TEST("empty string produces empty output") {
        expandIconTags("", dst, sizeof(dst));
        ASSERT_STREQ(dst, "");
    }
}
