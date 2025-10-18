#pragma once

#include <stdint.h>

#define TASK_SCHEDULE_FREQ      (50)

typedef struct
{
    uint16_t presc;
    void (*ext_callback)();
    uint16_t param_size;
    uint8_t * param;
} task_param_t;
