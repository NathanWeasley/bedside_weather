#pragma once

#include <stdint.h>

#define LED_HEIGHT          (16)
#define LED_WIDTH           (25)
#define LED_COLOR_DEPTH     (8)

#define LED_CNT             (LED_HEIGHT * LED_WIDTH)
#define LED_BUFFER_SIZE     (LED_CNT/8 + ((LED_CNT%8) == 0 ? 0 : 1))
#define LED_GRAYSCALE       (1 << LED_COLOR_DEPTH)

#define DEFAULT_BUFFER_SIZE (512)
#define UART_BUFFER_SIZE    (1024)