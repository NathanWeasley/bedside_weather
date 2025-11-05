#pragma once

#include "graphics/gfx_fonts.h"

#define ICON_16_HEIGHT      (16)
#define ICON_16_MAX_WIDTH   (16)
#define ICON_16_WIDTH_BYTE  (ICON_16_MAX_WIDTH / 8 + ((ICON_16_MAX_WIDTH % 8) != 0))

#define ICON_16_CNT         (10)

#define ICON_16_ASCII_BIAS  (' ')       // Dummy

namespace gfx
{

using Icon16 = Font<ICON_16_HEIGHT, ICON_16_MAX_WIDTH, WIDTH_FIXED, ICON_16_CNT, ICON_16_ASCII_BIAS>;

extern const Icon16 icon16_table;

}
