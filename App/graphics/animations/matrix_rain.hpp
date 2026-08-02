#pragma once

#include "config.h"

#include <cstdint>

namespace gfx::animations
{

/*
 * 过程式数字雨视图：只保存每一列的运动状态，不为各列分配独立画布。
 * 接口与 DisplayZone 的 View 约定兼容，可作为全屏预装载动画直接接入。
 */
class MatrixRainView
{
    struct ColumnState
    {
        int8_t head_y;
        uint8_t period_ticks;
        uint8_t ticks_left;
        uint8_t trail_length;
        uint8_t reset_delay;
        uint8_t peak_brightness;
    };

    ColumnState _columns[LED_WIDTH];

public:
    MatrixRainView();

    void reset();
    bool tick();
    void get_line(uint16_t line, uint8_t * output) const;

    static constexpr uint16_t x() { return 0U; }
    static constexpr uint16_t y() { return 0U; }
    static constexpr uint16_t width() { return LED_WIDTH; }
    static constexpr uint16_t height() { return LED_HEIGHT; }
};

} // namespace gfx::animations
