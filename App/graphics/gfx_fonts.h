#pragma once

#include <cstdint>
#include <type_traits>

enum WidthType : uint8_t
{
    WIDTH_FIXED = 0,
    WIDTH_VARIABLE = 1
};

template <
    uint8_t Height, uint8_t MaxWidth, WidthType WType,
    uint16_t Count, uint8_t ASCII_BIAS
>
struct Font;

template <
    uint8_t Height, uint8_t MaxWidth,
    uint16_t Count, uint8_t ASCII_BIAS
>
struct Font<Height, MaxWidth, WIDTH_FIXED, Count, ASCII_BIAS>
{
    static constexpr uint8_t __width_in_byte = MaxWidth/8 + (MaxWidth%8 != 0);

    using GlyphType = struct
    {
        uint8_t data[Height * __width_in_byte];
    };

    GlyphType glyphs[Count];

    const uint8_t * data(uint16_t ch_idx = 0) const { return glyphs[ch_idx].data; }

    static constexpr uint8_t width(uint16_t ch_idx = 0) { return MaxWidth; }
    static constexpr uint8_t height() { return Height; }
    static constexpr uint8_t widthByte() { return __width_in_byte; }
    static constexpr uint8_t ascii_bias() { return ASCII_BIAS; }
};

template <
    uint8_t Height, uint8_t MaxWidth,
    uint16_t Count, uint8_t ASCII_BIAS
>
struct Font<Height, MaxWidth, WIDTH_VARIABLE, Count, ASCII_BIAS>
{
    static constexpr uint8_t __width_in_byte = MaxWidth/8 + (MaxWidth%8 != 0);

    using GlyphType = struct
    {
        uint8_t width;
        uint8_t data[Height * __width_in_byte];
    };

    GlyphType glyphs[Count];

    const uint8_t * data(uint16_t ch_idx = 0) const { return glyphs[ch_idx].data; }
    uint8_t width(uint16_t ch_idx = 0) const { return glyphs[ch_idx].width; }

    static constexpr uint8_t height() { return Height; }
    static constexpr uint8_t widthByte() { return __width_in_byte; }
    static constexpr uint8_t ascii_bias() { return ASCII_BIAS; }
};

template <typename T>
struct is_Font : std::false_type {};

template <
    uint8_t Height, uint8_t MaxWidth, WidthType WType,
    uint16_t Count, uint8_t ASCII_BIAS
>
struct is_Font<Font<Height, MaxWidth, WType, Count, ASCII_BIAS>> : std::true_type {};

template <typename T>
static constexpr bool is_Font_v = is_Font<T>::value;
