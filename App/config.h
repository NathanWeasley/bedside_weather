#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LED_HEIGHT              (16)
#define LED_WIDTH               (25)
#define LED_COLOR_DEPTH         (8)

#define LED_CNT                 (LED_HEIGHT * LED_WIDTH)
#define LED_BUFFER_SIZE         (LED_CNT/8 + ((LED_CNT%8) == 0 ? 0 : 1))
#define LED_GRAYSCALE           (1 << LED_COLOR_DEPTH)

#define DEFAULT_RX_BUFFER_SIZE  (512)
#define DEFAULT_TX_BUFFER_SIZE  (64)
#define UART_BUFFER_SIZE        (1024)

#define GFX_VPAINT_MAX_WIDTH    (500)
#define GFX_VPAINT_MAX_HEIGHT   (32)

#ifdef __cplusplus
}
#endif
