#pragma once
#include <pgmspace.h>
#include <MD_MAX72xx.h>

// ─── 3×5 small font for date display ─────────────────────────────────────────
// Each glyph is 3 columns wide (variable), 5 rows tall, centered vertically
// in the 8-row display (1 blank row above, 2 blank rows below).
//
// Column byte: bit 0 = topmost LED row.
//   bit 0 → (blank row above glyph)
//   bit 1 → glyph row 0  (top)      = 2
//   bit 2 → glyph row 1             = 4
//   bit 3 → glyph row 2  (middle)   = 8
//   bit 4 → glyph row 3             = 16
//   bit 5 → glyph row 4  (bottom)   = 32
//   bits 6-7 → (blank rows below)
//
// Full column (all 5 rows on) = 2+4+8+16+32 = 62
//
// Format per char: <width>, <col0>, <col1>, [col2]
// Header: 'F', 1, firstChar, lastChar, height   (version 1 = single-byte ASCII range)

MD_MAX72XX::fontType_t PROGMEM _dateSmallFont[] =
{
  'F', 1, 32, 90, 8,

  // 32 ' '
  2, 0, 0,

  // 33–47: not used; zero entries keep the table contiguous
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  // ── Digits ───────────────────────────────────────────────────────────────────
  // Pixel layout: col0=left, col1=middle, col2=right; row top→bottom

  // '0'  OOO    col0=62, col1=34(top+bot), col2=62
  //      O O
  //      O O
  //      O O
  //      OOO
  3, 62, 34, 62,

  // '1'  .O     col0=32(bot), col1=62
  //       O
  //       O
  //       O
  //      OO
  2, 32, 62,

  // '2'  OOO    col0=58(top+mid+r3+bot), col1=42(top+mid+bot), col2=46(top+r1+mid+bot)
  //        O    Z-shape
  //      OOO
  //      O
  //      OOO
  3, 58, 42, 46,

  // '3'  OOO    col0=42(top+mid+bot), col1=42(top+mid+bot), col2=62(all)
  //        O
  //      OOO
  //        O
  //      OOO
  3, 42, 42, 62,

  // '4'  O O    col0=14(top+r1+mid), col1=8(mid), col2=62(all)
  //      O O
  //      OOO
  //        O
  //        O
  3, 14, 8, 62,

  // '5'  OOO    col0=46(top+r1+mid+bot), col1=42(top+mid+bot), col2=58(top+mid+r3+bot)
  //      O
  //      OOO
  //        O
  //      OOO
  3, 46, 42, 58,

  // '6'  OOO    col0=62(all), col1=42(top+mid+bot), col2=58(top+mid+r3+bot)
  //      O
  //      OOO
  //      O O
  //      OOO
  3, 62, 42, 58,

  // '7'  OOO    col0=2(top only), col1=2, col2=62
  //        O
  //        O
  //        O
  //        O
  3, 2, 2, 62,

  // '8'  OOO    col0=62, col1=42, col2=62
  //      O O
  //      OOO
  //      O O
  //      OOO
  3, 62, 42, 62,

  // '9'  OOO    col0=46(top+r1+mid+bot), col1=42(top+mid+bot), col2=62(all)
  //      O O
  //      OOO
  //        O
  //      OOO
  3, 46, 42, 62,

  // 58–64: not used
  0, 0, 0, 0, 0, 0, 0,

  // ── Letters A–Z ──────────────────────────────────────────────────────────────

  // 'A'  .O.    col0=62, col1=10(top+mid), col2=62
  //      O O
  //      OOO
  //      O O
  //      O O
  3, 62, 10, 62,

  // 'B'  OO.    col0=62, col1=42(top+mid+bot), col2=28(r1+mid+r3)
  //      O O
  //      OO.
  //      O O
  //      OO.
  3, 62, 42, 28,

  // 'C'  OOO    col0=62, col1=34(top+bot), col2=34
  //      O
  //      O
  //      O
  //      OOO
  3, 62, 34, 34,

  // 'D'  OO.    col0=62, col1=34, col2=28(r1+mid+r3 = inner arc)
  //      O O
  //      O O
  //      O O
  //      OO.
  3, 62, 34, 28,

  // 'E'  OOO    col0=62, col1=42(top+mid+bot), col2=34(top+bot)
  //      O
  //      OO.
  //      O
  //      OOO
  3, 62, 42, 34,

  // 'F'  OOO    col0=62, col1=10(top+mid), col2=2(top)
  //      O
  //      OO.
  //      O
  //      O
  3, 62, 10, 2,

  // 'G'  OOO    col0=62, col1=42, col2=56(mid+r3+bot)
  //      O
  //      O OO
  //      O O
  //      OOO
  3, 62, 42, 56,

  // 'H'  O O    col0=62, col1=8(mid), col2=62
  //      O O
  //      OOO
  //      O O
  //      O O
  3, 62, 8, 62,

  // 'I'  OOO    col0=34(top+bot), col1=62, col2=34
  //       O
  //       O
  //       O
  //      OOO
  3, 34, 62, 34,

  // 'J'  OOO    col0=32(bot), col1=34(top+bot), col2=62
  //        O
  //        O
  //      O O
  //      OOO
  3, 32, 34, 62,

  // 'K'  O O    col0=62, col1=8(mid), col2=54(top+r1+r3+bot)
  //      O O
  //      OO
  //      O O
  //      O O
  3, 62, 8, 54,

  // 'L'  O      col0=62, col1=32(bot), col2=32
  //      O
  //      O
  //      O
  //      OOO
  3, 62, 32, 32,

  // 'M'  O O    col0=62, col1=6(top+r1), col2=62
  //      OOO
  //      O O
  //      O O
  //      O O
  3, 62, 6, 62,

  // 'N'  O O    col0=62, col1=14(top+r1+mid), col2=62
  //      OO O
  //      O O
  //      O O
  //      O O
  3, 62, 14, 62,

  // 'O'  OOO    col0=62, col1=34, col2=62
  //      O O
  //      O O
  //      O O
  //      OOO
  3, 62, 34, 62,

  // 'P'  OOO    col0=62, col1=10(top+mid), col2=14(top+r1+mid)
  //      O O
  //      OO.
  //      O
  //      O
  3, 62, 10, 14,

  // 'Q'  OOO    col0=62, col1=50(top+r3+bot), col2=62
  //      O O
  //      O O
  //      OO
  //      OOO
  3, 62, 50, 62,

  // 'R'  OO.    col0=62, col1=8(mid), col2=54
  //      O O
  //      OO.
  //      O O
  //      O O
  3, 62, 8, 54,

  // 'S'  OOO    col0=14(top+r1+mid), col1=42(top+mid+bot), col2=56(mid+r3+bot)
  //      O
  //      OOO
  //        O
  //      OOO
  3, 14, 42, 56,

  // 'T'  OOO    col0=2(top), col1=62, col2=2
  //       O
  //       O
  //       O
  //       O
  3, 2, 62, 2,

  // 'U'  O O    col0=62, col1=32(bot), col2=62
  //      O O
  //      O O
  //      O O
  //      OOO
  3, 62, 32, 62,

  // 'V'  O O    col0=30(top+r1+mid+r3), col1=32(bot), col2=30
  //      O O
  //      O O
  //      O O
  //       O
  3, 30, 32, 30,

  // 'W'  O O    col0=62, col1=48(r3+bot), col2=62
  //      O O
  //      O O
  //      OOO
  //      O O
  3, 62, 48, 62,

  // 'X'  O O    col0=54(top+r1+r3+bot), col1=8(mid), col2=54
  //      O O
  //       O
  //      O O
  //      O O
  3, 54, 8, 54,

  // 'Y'  O O    col0=6(top+r1), col1=62, col2=6
  //      O O
  //       O
  //       O
  //       O
  3, 6, 62, 6,

  // 'Z'  OOO    col0=50(top+r3+bot), col1=42(top+mid+bot), col2=38(top+r1+bot)
  //        O    diagonal from top-right to bottom-left
  //       O
  //      O
  //      OOO
  3, 50, 42, 38,
};
