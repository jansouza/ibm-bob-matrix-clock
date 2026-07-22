#pragma once

/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */
#include <stddef.h>

// Convert a UTF-8 string to Latin-1 (ISO 8859-1).
// Characters outside Latin-1 (code-point > 255) are replaced with '?'.
// dst is always null-terminated. Returns dst.
char* utf8ToLatin1(const char* src, char* dst, size_t maxLen);

// Convert a Latin-1 (ISO 8859-1) string to UTF-8.
// dst must be large enough to hold up to 2× the source length + 1.
// dst is always null-terminated. Returns dst.
char* latin1ToUtf8(const char* src, char* dst, size_t maxLen);

// Scan src for [name] icon tags (e.g. "[heart]", "[warn]") and replace each
// with the mapped CP437 special-glyph byte (0x01–0x1F range, rendered
// directly by MD_MAX72XX's default font). Unknown tags are copied through
// verbatim, brackets included. dst is always null-terminated. Returns dst.
char* expandIconTags(const char* src, char* dst, size_t maxLen);
