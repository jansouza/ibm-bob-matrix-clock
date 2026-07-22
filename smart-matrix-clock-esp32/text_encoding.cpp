#include "text_encoding.h"
#include <stdint.h>

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
