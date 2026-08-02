#pragma once

#include "tasks/task.h"

#define DISPLAY_TASK_PRESC             (5)

constexpr uint8_t DISPLAY_LINE_COUNT = 3U;
constexpr uint8_t DISPLAY_LINE_TEXT_CAPACITY = 40U;

/* MainTask 生成最终文本，DisplayTask 只负责绘制与动画。 */
struct DisplayContent
{
    char lines[DISPLAY_LINE_COUNT][DISPLAY_LINE_TEXT_CAPACITY];
};

class DisplayTask
    : public TaskBase
{
    using Base = TaskBase;

    DisplayContent _content;
    uint8_t _dirty_lines;

    DisplayTask()
    : Base(DISPLAY_TASK_PRESC)
    , _content{}
    , _dirty_lines((1U << DISPLAY_LINE_COUNT) - 1U)
    {}
    ~DisplayTask() = default;

public:
    static inline DisplayTask * instance()
    {
        static DisplayTask tsk;
        return &tsk;
    }

    void init() override;
    void tick() override;

    void set_display_content(const DisplayContent& content);
};
