#pragma once

#include <stdint.h>

#define ICON_5_HEIGHT      (5)
#define ICON_5_MAX_WIDTH   (8)
#define ICON_5_WIDTH_BYTE  (ICON_5_MAX_WIDTH / 8 + ((ICON_5_MAX_WIDTH % 8) != 0))

#define ICON_5_CNT         (128)

#define ICON_5_ASCII_BIAS  (' ')

typedef struct
{
    uint8_t width;
    uint8_t data[ICON_5_HEIGHT * ICON_5_WIDTH_BYTE];
} icon_5_glyph_t;

typedef struct
{
    const uint8_t height;
    icon_5_glyph_t glyves[ICON_5_CNT];
} icon_5_font_t;

extern const icon_5_font_t g_font5;