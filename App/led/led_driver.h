#pragma once

#include <stdint.h>
#include "stm32g0xx_ll_gpio.h"
#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t data[LED_BUFFER_SIZE];
} led_slice_t;

typedef struct
{
    led_slice_t slice[LED_COLOR_DEPTH];
} led_breakdown_t;

void led_init(void);
void led_update_img(const uint8_t * pdata);
void led_start_refresh(void);
void led_refresh_timer_irq_handler(void);
uint32_t led_get_refresh_error_count(void);

#ifdef __cplusplus
}
#endif
