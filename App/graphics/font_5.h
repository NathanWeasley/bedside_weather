#pragma once

#include "graphics/gfx_fonts.h"

#define FONT_5_HEIGHT      (5)
#define FONT_5_MAX_WIDTH   (8)
#define FONT_5_WIDTH_BYTE  (FONT_5_MAX_WIDTH / 8 + ((FONT_5_MAX_WIDTH % 8) != 0))

#define FONT_5_CNT         (128)

#define FONT_5_ASCII_BIAS  (' ')

using Font5 = Font<FONT_5_HEIGHT, FONT_5_MAX_WIDTH, WIDTH_VARIABLE, FONT_5_CNT, FONT_5_ASCII_BIAS>;

extern const Font5 font5_table;

