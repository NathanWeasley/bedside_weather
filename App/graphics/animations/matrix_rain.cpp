#include "graphics/animations/matrix_rain.hpp"

#include <cstring>

namespace gfx::animations
{
namespace
{

static_assert(LED_WIDTH == 25U, "数字雨预设参数需要与屏幕列数一致");

/* 每列使用固定但互不相同的初相位，避免上电时同步下落。 */
constexpr int8_t INITIAL_HEAD[LED_WIDTH] =
{
    -8, 3, 13, -2, 7, -12, 11, 0, 15, -5,
    5, -10, 9, 1, 14, -7, 6, -3, 12, 2,
    -11, 8, -1, 10, 4
};

/* period=1 表示每个 DisplayTask 动画节拍移动一个像素。 */
constexpr uint8_t PERIOD_TICKS[LED_WIDTH] =
{
    1, 2, 1, 3, 2, 1, 4, 2, 1, 3,
    2, 1, 3, 2, 1, 4, 2, 1, 3, 2,
    1, 3, 2, 4, 1
};

constexpr uint8_t TRAIL_LENGTH[LED_WIDTH] =
{
    7, 5, 9, 4, 6, 8, 5, 7, 10, 4,
    6, 9, 5, 8, 7, 4, 6, 10, 5, 8,
    7, 4, 9, 6, 5
};

constexpr uint8_t RESET_DELAY[LED_WIDTH] =
{
    5, 11, 3, 8, 14, 6, 10, 4, 12, 7,
    2, 13, 5, 9, 3, 15, 6, 11, 4, 8,
    12, 5, 10, 7, 3
};

constexpr uint8_t PEAK_BRIGHTNESS[LED_WIDTH] =
{
    255, 224, 240, 208, 232, 255, 216, 244, 255, 220,
    236, 248, 212, 240, 255, 224, 232, 252, 216, 244,
    255, 220, 236, 228, 248
};

/* 输入值会经过现有 Gamma LUT，尾部避免使用会被映射为零的过低亮度。 */
constexpr uint8_t TRAIL_BRIGHTNESS[] =
{
    255, 228, 204, 180, 156, 132, 108, 84, 64, 44
};

constexpr uint8_t MAX_TRAIL_LENGTH =
    static_cast<uint8_t>(sizeof(TRAIL_BRIGHTNESS) / sizeof(TRAIL_BRIGHTNESS[0]));

} // namespace

MatrixRainView::MatrixRainView()
{
    reset();
}

void MatrixRainView::reset()
{
    for (uint8_t column = 0U; column < LED_WIDTH; ++column)
    {
        ColumnState& state = _columns[column];
        state.head_y = INITIAL_HEAD[column];
        state.period_ticks = PERIOD_TICKS[column];
        state.ticks_left = state.period_ticks;
        state.trail_length = TRAIL_LENGTH[column];
        if (state.trail_length > MAX_TRAIL_LENGTH)
        {
            state.trail_length = MAX_TRAIL_LENGTH;
        }
        state.reset_delay = RESET_DELAY[column];
        state.peak_brightness = PEAK_BRIGHTNESS[column];
    }
}

bool MatrixRainView::tick()
{
    bool changed = false;

    for (uint8_t column = 0U; column < LED_WIDTH; ++column)
    {
        ColumnState& state = _columns[column];
        if (state.ticks_left > 1U)
        {
            --state.ticks_left;
            continue;
        }

        state.ticks_left = state.period_ticks;
        ++state.head_y;
        if (state.head_y >= static_cast<int16_t>(LED_HEIGHT + state.trail_length))
        {
            state.head_y = -static_cast<int8_t>(state.reset_delay);
        }
        changed = true;
    }

    return changed;
}

void MatrixRainView::get_line(uint16_t line, uint8_t * output) const
{
    if (output == nullptr)
    {
        return;
    }

    std::memset(output, 0, LED_WIDTH);
    if (line >= LED_HEIGHT)
    {
        return;
    }

    for (uint8_t column = 0U; column < LED_WIDTH; ++column)
    {
        const ColumnState& state = _columns[column];
        const int16_t distance = static_cast<int16_t>(state.head_y) -
                                 static_cast<int16_t>(line);
        if ((distance < 0) || (distance >= state.trail_length))
        {
            continue;
        }

        const uint16_t brightness =
            static_cast<uint16_t>(TRAIL_BRIGHTNESS[distance]) * state.peak_brightness;
        output[column] = static_cast<uint8_t>((brightness + 127U) / 255U);
    }
}

} // namespace gfx::animations
