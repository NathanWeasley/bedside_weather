#include "config.h"
#include "tasks/task.h"
#include "tasks/task_schedule.h"
#include "tasks/comm_task.h"
#include "tasks/display_task.h"

#include <cstddef>

typedef struct
{
    uint8_t task_cnt;
    TaskBase* ptasks[TASK_MAX_TASKS];
} task_schedule_t;

static volatile uint32_t g_pending_schedule_ticks = 0;
static task_schedule_t g_task_list = {};

namespace
{
uint32_t enter_critical_section();
void leave_critical_section(uint32_t primask);
}

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
    if (g_pending_schedule_ticks != 0xFFFFFFFFUL)
    {
        ++g_pending_schedule_ticks;
    }
}

extern "C" void scheduler_run()
{
    /* 在执行任务之前消费一个节拍；任务期间到达的新节拍会留给下一轮。 */
    const uint32_t primask = enter_critical_section();
    if (g_pending_schedule_ticks == 0U)
    {
        leave_critical_section(primask);
        return;
    }
    --g_pending_schedule_ticks;
    leave_critical_section(primask);

    for (uint8_t i = 0; i < g_task_list.task_cnt; ++i)
    {
        if (g_task_list.ptasks[i]->test_tick())
        {
            g_task_list.ptasks[i]->tick();
        }
    }

}









namespace
{

uint32_t enter_critical_section()
{
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void leave_critical_section(uint32_t primask)
{
    if (primask == 0)
    {
        __enable_irq();
    }
}

[[noreturn]] void cpp_runtime_fault()
{
    __disable_irq();
    while (1)
    {
    }
}

} // namespace

extern "C" void __cxa_pure_virtual()
{
    cpp_runtime_fault();
}

/*
 * ARM EABI 的局部静态对象 guard。第 0 字节表示构造完成，第 1 字节表示构造中。
 * 当前工程没有异常和多线程，但仍需保证 instance() 不会重复构造单例。
 */
extern "C" int __cxa_guard_acquire(int * guard)
{
    volatile uint8_t * state = reinterpret_cast<volatile uint8_t *>(guard);
    const uint32_t primask = enter_critical_section();

    if (state[0] != 0)
    {
        leave_critical_section(primask);
        return 0;
    }

    if (state[1] != 0)
    {
        leave_critical_section(primask);
        cpp_runtime_fault();
    }

    state[1] = 1;
    leave_critical_section(primask);
    return 1;
}

extern "C" void __cxa_guard_release(int * guard)
{
    volatile uint8_t * state = reinterpret_cast<volatile uint8_t *>(guard);
    const uint32_t primask = enter_critical_section();
    state[0] = 1;
    state[1] = 0;
    leave_critical_section(primask);
}

extern "C" void __cxa_guard_abort(int * guard)
{
    volatile uint8_t * state = reinterpret_cast<volatile uint8_t *>(guard);
    const uint32_t primask = enter_critical_section();
    state[1] = 0;
    leave_critical_section(primask);
}

// 当前固件禁止动态分配；若后续代码误用 new，则确定性停机而不是返回空指针制造未定义行为。
void * operator new(std::size_t) { cpp_runtime_fault(); }
void * operator new[](std::size_t) { cpp_runtime_fault(); }
void operator delete(void*) {}
void operator delete[](void*) {}
void operator delete(void*, std::size_t) {}
void operator delete[](void*, std::size_t) {}
