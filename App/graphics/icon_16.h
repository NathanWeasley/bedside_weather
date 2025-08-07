#pragma once

#include <stdint.h>

#define ICON_16_HEIGHT      (16)
#define ICON_16_MAX_WIDTH   (16)
#define ICON_16_WIDTH_BYTE  (ICON_16_MAX_WIDTH / 8 + ((ICON_16_MAX_WIDTH % 8) != 0))

#define ICON_16_CNT         (8)

typedef struct
{
    uint8_t width;
    uint8_t data[ICON_16_HEIGHT * ICON_16_WIDTH_BYTE];
} icon_16_glyph_t;

typedef struct
{
    const uint8_t height;
    icon_16_glyph_t glyves[ICON_16_CNT];
} icon_16_font_t;

extern const icon_16_font_t g_font16;
