#pragma once

#include "config.h"
#include <stdint.h>

typedef struct
{
    uint8_t * p0;
    uint8_t * p1;
    uint8_t * phead;
    uint8_t * ptail;
    uint8_t data[DEFAULT_BUFFER_SIZE];
} circular_buffer_t;

void cbuf_init(circular_buffer_t * pbuf);
size_t cbuf_size(circular_buffer_t * pbuf);
void cbuf_write(uint8_t * pdata, size_t len);