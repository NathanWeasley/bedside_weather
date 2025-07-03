#pragma once

#include <stdint.h>
#include "stm32g0xx_ll_gpio.h"
#include "App/config.h"

uint32_t led_get_txbuf_addr();
uint32_t led_get_txbuf_size();
