#pragma once

#include "tasks/task.h"

#define MAIN_TASK_PRESC             (1)

class MainTask
    : public TaskBase
{
    using Base = TaskBase;

    uint16_t        _entry_cnt;

    MainTask()
    : Base(MAIN_TASK_PRESC)
    , _entry_cnt(0)
    {}
    ~MainTask() = default;

public:
    static inline MainTask * instance()
    {
        static MainTask tsk;
        return &tsk;
    }

    void init() override;
    void tick() override;

private:

};




