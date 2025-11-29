#include "config.h"
#include "tasks/task.h"
#include "tasks/task_schedule.h"
#include "tasks/comm_task.h"
#include "tasks/display_task.h"

typedef struct
{
    uint8_t task_cnt;
    TaskBase* ptasks[TASK_MAX_TASKS];
} task_schedule_t;

static uint8_t g_start_schedule = 0;
static task_schedule_t g_task_list = { 0 };

extern "C" void scheduler_init()
{
    g_task_list.task_cnt = 2;
    g_task_list.ptasks[0] = CommTask::instance();
    g_task_list.ptasks[1] = DisplayTask::instance();

    // Initialize all tasks
    for (uint8_t i = 0; i < g_task_list.task_cnt; ++i)
    {
        g_task_list.ptasks[i]->init();
    }
}

extern "C" void scheduler_arm()
{
    g_start_schedule = 1;
}

extern "C" void scheduler_run()
{
    if (!g_start_schedule)
    {
        // Not ready for another run
        return;
    }

    for (uint8_t i = 0; i < g_task_list.task_cnt; ++i)
    {
        if (g_task_list.ptasks[i]->test_tick())
        {
            g_task_list.ptasks[i]->tick();
        }
    }

    g_start_schedule = 0;
}

