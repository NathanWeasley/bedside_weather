#include "led/led_driver.h"
#include <string.h>

static led_slice_t      g_led_tx_buf;
static led_breakdown_t  g_led_breakdown;

static uint16_t         g_weight_table[LED_COLOR_DEPTH];
static uint16_t         g_current_tick_count;
static uint8_t          g_current_bit;

static const uint8_t    g_gamma_lut[LED_GRAYSCALE] =
{
    0,
};

void led_init()
{
    uint16_t i;

    /** Initializing global variables */

    g_led_tx_buf.head = g_led_tx_buf.data;
    g_led_tx_buf.mid = g_led_tx_buf.head + LED_BUFFER_SIZE/2;

    memset(&g_led_breakdown, 0xFF, sizeof(g_led_breakdown));
    for (i = 0; i < LED_COLOR_DEPTH; ++i)
    {
        g_led_breakdown.slice[i].head = g_led_breakdown.slice[i].data;
        g_led_breakdown.slice[i].mid = g_led_breakdown.slice[i].head + LED_BUFFER_SIZE/2;
    }
    ///< Debug
    g_led_breakdown.slice[0].data[0] = 0x7F;
    g_led_breakdown.slice[1].data[0] = 0xBF;
    g_led_breakdown.slice[2].data[0] = 0xDF;
    g_led_breakdown.slice[3].data[0] = 0xEF;
    g_led_breakdown.slice[4].data[0] = 0xF7;
    g_led_breakdown.slice[5].data[0] = 0xFB;
    g_led_breakdown.slice[6].data[0] = 0xFD;
    g_led_breakdown.slice[7].data[0] = 0xFE;

    for (i = 0; i < LED_COLOR_DEPTH; ++i)
    {
        g_weight_table[i] = 1 << i;
    };

    g_current_bit = 0;
    g_current_tick_count = 0;

    /** load test image */

    led_update_img(g_test_img);

    /** Copy first slice */

    memcpy(g_led_tx_buf.head, g_led_breakdown.slice[0].head, LED_BUFFER_SIZE);
}

void led_update_img(const uint8_t * pdata)
{
    uint8_t i, j, k, mask;
    const uint8_t * ptrbuf = pdata;

    ;
}

uint32_t led_get_txbuf_addr()
{
    return (uint32_t)g_led_tx_buf.head;
}

uint32_t led_get_txbuf_size()
{
    return LED_BUFFER_SIZE;
}

void led_copy_first_half()
{
    memcpy(g_led_tx_buf.head, g_led_breakdown.slice[g_current_bit].head, LED_BUFFER_SIZE/2);
}

void led_copy_last_half()
{
    memcpy(g_led_tx_buf.mid, g_led_breakdown.slice[g_current_bit].mid, LED_BUFFER_SIZE/2);
}

void led_next_tick()
{
    if (++g_current_tick_count == g_weight_table[g_current_bit])
    {
        g_current_tick_count = 0;
        if (++g_current_bit == LED_COLOR_DEPTH)
        {
            g_current_bit = 0;
        }
    }
}
