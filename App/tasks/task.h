#pragma once

#include <stdint.h>

typedef struct
{
    uint16_t presc;
    void (*ext_callback)();
    uint16_t param_size;
    uint8_t * param;
} task_param_t;
