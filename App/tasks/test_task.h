#pragma once

#include "tasks/task.h"

/**
 * Task APIs 
 */

#ifdef __cplusplus
extern "C"
{
#endif

void test_task_init(task_param_t * param);
void test_task_tick();



#ifdef __cplusplus
}
#endif
