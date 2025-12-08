#include "tasks/main_task.h"
#include "tasks/comm_task.h"
#include "tasks/display_task.h"

#include "rtc.h"

void MainTask::init()
{
    ;
}

void MainTask::tick()
{
    if (++_entry_cnt == 10000)
    {
        _entry_cnt = 0;
    }

    ;
}
