#pragma once
#include <cstdint>
#include "lgfx/utility/pgmspace.h"

namespace adv {
inline uint8_t font_byte(const uint8_t* ptr)
{
    return pgm_read_byte(ptr);
}

inline uint16_t font_u16(const uint8_t* ptr)
{
    return (static_cast<uint16_t>(font_byte(ptr)) << 8) | font_byte(ptr + 1);
}

inline const uint8_t* find_efont_glyph(const uint8_t* base, uint16_t encoding)
{
    const uint8_t* font = &base[23];
    uint16_t start_upper = font_u16(&base[17]);
    uint16_t start_lower = font_u16(&base[19]);
    uint16_t start_unicode = font_u16(&base[21]);

    if (encoding <= 255) {
        if (encoding >= 'a') {
            font += start_lower;
        } else if (encoding >= 'A') {
            font += start_upper;
        }
        for (; font_byte(font + 1) != 0; font += font_byte(font + 1)) {
            if (font_byte(font) == encoding) {
                return font + 2;
            }
        }
        return nullptr;
    }

    font += start_unicode;
    const uint8_t* unicode_lut = font;
    uint16_t current = 0;
    do {
        font += font_u16(unicode_lut);
        current = font_u16(unicode_lut + 2);
        unicode_lut += 4;
    } while (current < encoding);

    for (; (current = font_u16(font)) != 0; font += font_byte(font + 2)) {
        if (current == encoding) {
            return font + 3;
        }
    }
    return nullptr;
}

// The Japanese efont subset omits halfwidth katakana. Reuse the matching
// fullwidth glyph, compressed horizontally by the renderer into one cell.
inline uint16_t japanese_halfwidth_glyph(uint16_t cp) {
    static constexpr uint16_t map[] = {
        0x3002, 0x300c, 0x300d, 0x3001, 0x30fb, 0x30f2, 0x30a1, 0x30a3, 0x30a5,
        0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3, 0x30fc, 0x30a2, 0x30a4,
        0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad, 0x30af, 0x30b1, 0x30b3, 0x30b5,
        0x30b7, 0x30b9, 0x30bb, 0x30bd, 0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8,
        0x30ca, 0x30cb, 0x30cc, 0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8,
        0x30db, 0x30de, 0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8,
        0x30e9, 0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c,
    };
    return cp >= 0xff61 && cp <= 0xff9f ? map[cp - 0xff61] : cp;
}

} // namespace adv
