#include "utils/cbuffer.h"
#include <string.h>

static circular_buffer_t g_buffer;

void cbuf_init(circular_buffer_t * pbuf)
{
    memset(pbuf, 0, sizeof(circular_buffer_t));

    pbuf->p0 = pbuf->data;
    pbuf->p1 = pbuf->p0 + DEFAULT_BUFFER_SIZE;
    pbuf->phead = pbuf->p0;
    pbuf->ptail = pbuf->p0;
}

size_t cbuf_size(circular_buffer_t * pbuf)
{
    if (pbuf->phead >= pbuf->ptail)
    {
        return pbuf->phead - pbuf->ptail;
    }
    else
    {
        return (pbuf->p1 - pbuf->ptail) + (pbuf->phead - pbuf->p0);
    }
}

void cbuf_write(uint8_t * pdata, size_t len)
{
    
}
