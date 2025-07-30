#pragma once

#include "config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t data[LED_CNT];
} gfx_img_t;

extern const uint8_t g_test_img[];


/** These functions directly draw on image buffer */
void gfx_update_img(const uint8_t * pimg);
uint16_t gfx_draw_px(uint16_t x, uint16_t y, uint8_t color);
uint16_t gfx_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
uint16_t gfx_draw_char_3x5();


/** C wrappers for CPP functions */











#ifdef __cplusplus
}
#endif
