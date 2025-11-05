#include "utils/cbuffer.h"
#include <string.h>


/**
 * All pointer validity checks are omitted.
 */

void circular_buffer_init(circular_buffer_t * cb)
{
    cb->head = 0;
    cb->tail = 0;
}

uint8_t circular_buffer_is_empty(const circular_buffer_t * cb)
{
    return cb->head == cb->tail;
}

uint16_t circular_buffer_available(const circular_buffer_t * cb)
{
    if (cb->head >= cb->tail)
    {
        return cb->head - cb->tail;
    }
    else
    {
        return DEFAULT_RX_BUFFER_SIZE - cb->tail  + cb->head;
    }
}

uint8_t circular_buffer_read_byte(circular_buffer_t * cb, uint8_t * byte)
{
    if (circular_buffer_is_empty(cb))
    {
        return 0;
    }

    *byte = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % DEFAULT_RX_BUFFER_SIZE;

    return 1;
}

void circular_buffer_write_byte(circular_buffer_t * cb, uint8_t byte)
{
    cb->buffer[cb->head] = byte;
    cb->head = (cb->head + 1) % DEFAULT_RX_BUFFER_SIZE;

    if (cb->head == cb->tail)
    {
        cb->tail = (cb->tail + 1) % DEFAULT_RX_BUFFER_SIZE;
    }
}
