#include "epaper_ui/bitmap_font.h"

#include <algorithm>

namespace epaper_font {
namespace {
// The generator appends these glyphs in this exact order after ASCII.
constexpr uint32_t kPolish[] = {
    0x0104, 0x0105, 0x0106, 0x0107, 0x0118, 0x0119, 0x0141, 0x0142,
    0x0143, 0x0144, 0x00d3, 0x00f3, 0x015a, 0x015b, 0x0179, 0x017a,
    0x017b, 0x017c,
};
}

uint32_t NextCodepoint(std::string_view text, size_t& offset) {
    if (offset >= text.size()) return 0;
    const auto first = static_cast<unsigned char>(text[offset++]);
    if (first < 0x80) return first;
    unsigned count = 0;
    uint32_t value = 0;
    uint32_t minimum = 0;
    if (first >= 0xc2 && first <= 0xdf) { count = 1; value = first & 0x1f; minimum = 0x80; }
    else if (first >= 0xe0 && first <= 0xef) { count = 2; value = first & 0x0f; minimum = 0x800; }
    else if (first >= 0xf0 && first <= 0xf4) { count = 3; value = first & 7; minimum = 0x10000; }
    else return '?';
    if (offset + count > text.size()) return '?';
    for (unsigned i = 0; i < count; ++i) {
        const auto next = static_cast<unsigned char>(text[offset + i]);
        if ((next & 0xc0) != 0x80) return '?';
        value = (value << 6) | (next & 0x3f);
    }
    offset += count;
    if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return '?';
    return value;
}

const GlyphBitmap* FindGlyph(const BitmapFont& font, uint32_t codepoint) {
    if (codepoint >= font.first_char && codepoint <= font.last_char)
        return &font.glyphs[codepoint - font.first_char];
    const size_t base = static_cast<size_t>(font.last_char - font.first_char + 1);
    for (size_t i = 0; i < sizeof(kPolish) / sizeof(kPolish[0]) && i < font.polish_glyph_count; ++i)
        if (codepoint == kPolish[i]) return &font.glyphs[base + i];
    if ('?' >= font.first_char && '?' <= font.last_char) return &font.glyphs['?' - font.first_char];
    return nullptr;
}

int MeasureText(const BitmapFont& font, std::string_view text, int tracking) {
    int width = 0;
    bool first = true;
    size_t offset = 0;
    while (offset < text.size()) {
        const auto ch = NextCodepoint(text, offset);
        if (ch == '\r' || ch == '\n') break;
        const GlyphBitmap* glyph = FindGlyph(font, ch);
        if (!glyph) continue;
        if (!first) width += std::max(tracking, 0);
        width += glyph->advance;
        first = false;
    }
    return width;
}
}
