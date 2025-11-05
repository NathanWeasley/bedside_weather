#pragma once

#include "config.h"
#include <stdint.h>

typedef struct
{
    uint8_t buffer[DEFAULT_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} circular_buffer_t;

void circular_buffer_init(circular_buffer_t * cb);

uint8_t circular_buffer_is_empty(const circular_buffer_t * cb);

uint16_t circular_buffer_available(const circular_buffer_t * cb);

uint8_t circular_buffer_read_byte(circular_buffer_t * cb, uint8_t * byte);

void circular_buffer_write_byte(circular_buffer_t * cb, uint8_t byte);
