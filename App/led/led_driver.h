#pragma once

#include <stdint.h>
#include "stm32g0xx_ll_gpio.h"
#include "config.h"

extern const uint8_t g_test_img[];

typedef struct
{
    uint8_t * head;
    uint8_t * mid;
    uint8_t data[LED_BUFFER_SIZE];
} led_slice_t;

typedef struct
{
    led_slice_t slice[LED_COLOR_DEPTH];
} led_breakdown_t;

void led_init();
void led_update_img(const uint8_t * pdata);

uint32_t led_get_txbuf_addr();
uint32_t led_get_txbuf_size();
void led_copy_first_half();
void led_copy_last_half();
void led_next_tick();
