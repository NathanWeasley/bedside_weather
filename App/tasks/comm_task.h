#pragma once

#include "tasks/task.h"

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t comm_task_param_size();

void comm_task_init(task_param_t * param);
void comm_task_tick();


#ifdef __cplusplus
}
#endif

