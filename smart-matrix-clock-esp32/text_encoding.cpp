/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include "text_encoding.h"
#include <stdint.h>
#include <string.h>

// ─── UTF-8 → Latin-1 ──────────────────────────────────────────────────────────
// Decodes each UTF-8 sequence to its Unicode code-point, then maps to Latin-1.
// Code-points 0x00–0xFF map 1:1 to Latin-1; anything above becomes '?'.

char* utf8ToLatin1(const char* src, char* dst, size_t maxLen) {
    if (!src || !dst || maxLen == 0) return dst;

    size_t out = 0;
    const unsigned char* s = (const unsigned char*)src;

    while (*s && out < maxLen - 1) {
        uint32_t cp;

        if (*s < 0x80) {
            // 1-byte sequence (ASCII)
            cp = *s++;
        } else if ((*s & 0xE0) == 0xC0) {
            // 2-byte sequence
            cp = (*s++ & 0x1F) << 6;
            if ((*s & 0xC0) == 0x80) cp |= (*s++ & 0x3F); else cp = '?';
        } else if ((*s & 0xF0) == 0xE0) {
            // 3-byte sequence
            cp = (*s++ & 0x0F) << 12;
            if ((*s & 0xC0) == 0x80) { cp |= (*s++ & 0x3F) << 6; } else { cp = '?'; s += 2; }
            if ((*s & 0xC0) == 0x80) { cp |= (*s++ & 0x3F); }      else { cp = '?'; s++; }
        } else {
            // 4-byte or invalid — skip
            cp = '?';
            s++;
            while ((*s & 0xC0) == 0x80) s++;
        }

        dst[out++] = (cp <= 0xFF) ? (char)(uint8_t)cp : '?';
    }

    dst[out] = '\0';
    return dst;
}

// ─── Icon tags → CP437 special glyphs ────────────────────────────────────────
// Tags are matched literally (case-sensitive, no nesting). Maps to control
// bytes 0x01–0x1F, which MD_MAX72XX's default font renders as CP437 symbols.

struct IconTag {
    const char* name;
    char        code;
};

static const IconTag _iconTags[] = {
    { "heart",       0x03 },
    { "diamond",     0x04 },
    { "club",        0x05 },
    { "spade",       0x06 },
    { "bullet",      0x07 },
    { "smile",       0x01 },
    { "star",        0x0F },
    { "arrow_right", 0x10 },
    { "arrow_left",  0x11 },
    { "note",        0x0E },
};
static const size_t _iconTagCount = sizeof(_iconTags) / sizeof(_iconTags[0]);

// Look up "name" (length nameLen, no brackets) in the icon table.
// Returns the mapped byte, or 0 if not found.
static char _lookupIconTag(const char* name, size_t nameLen) {
    for (size_t i = 0; i < _iconTagCount; i++) {
        if (strlen(_iconTags[i].name) == nameLen &&
            strncmp(_iconTags[i].name, name, nameLen) == 0) {
            return _iconTags[i].code;
        }
    }
    return 0;
}

char* expandIconTags(const char* src, char* dst, size_t maxLen) {
    if (!src || !dst || maxLen == 0) return dst;

    size_t out = 0;
    const char* s = src;

    while (*s && out < maxLen - 1) {
        if (*s == '[') {
            const char* close = strchr(s + 1, ']');
            if (close) {
                size_t nameLen = (size_t)(close - s - 1);
                char code = _lookupIconTag(s + 1, nameLen);
                if (code != 0) {
                    dst[out++] = code;
                    s = close + 1;
                    continue;
                }
            }
        }
        dst[out++] = *s++;
    }

    dst[out] = '\0';
    return dst;
}

// ─── Latin-1 → UTF-8 ──────────────────────────────────────────────────────────
// Latin-1 bytes 0x00–0x7F are single-byte UTF-8 (identical).
// Latin-1 bytes 0x80–0xFF each expand to a 2-byte UTF-8 sequence.

char* latin1ToUtf8(const char* src, char* dst, size_t maxLen) {
    if (!src || !dst || maxLen == 0) return dst;

    size_t out = 0;
    const unsigned char* s = (const unsigned char*)src;

    while (*s) {
        if (*s < 0x80) {
            if (out + 1 >= maxLen) break;
            dst[out++] = (char)*s++;
        } else {
            // 2-byte UTF-8: 110xxxxx 10xxxxxx
            if (out + 2 >= maxLen) break;
            dst[out++] = (char)(0xC0 | (*s >> 6));
            dst[out++] = (char)(0x80 | (*s & 0x3F));
            s++;
        }
    }

    dst[out] = '\0';
    return dst;
}
