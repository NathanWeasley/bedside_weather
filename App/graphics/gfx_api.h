#pragma once

#include "config.h"
#include <stdint.h>

typedef struct
{
    uint8_t data[LED_CNT];
} gfx_img_t;

extern const uint8_t g_test_img[];


/** These functions directly draw on image buffer */
void gfx_update_img(const uint8_t * pimg);
void gfx_update_img(uint8_t * pmem, const uint8_t * pimg);


/** C wrappers for CPP functions */












