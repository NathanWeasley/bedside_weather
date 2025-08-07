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

void test_task_update_icon();


#ifdef __cplusplus
}
#endif
