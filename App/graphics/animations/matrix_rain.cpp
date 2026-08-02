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

/* 原 100 ms 动画节拍下的移动周期，用于换算 100 FPS 下的 Q8.8 速度。 */
constexpr uint8_t PERIOD_TICKS[LED_WIDTH] =
{
    1, 2, 1, 3, 2, 1, 4, 2, 1, 3,
    2, 1, 3, 2, 1, 4, 2, 1, 3, 2,
    1, 3, 2, 4, 1
};

constexpr uint8_t TRAIL_LENGTH[LED_WIDTH] =
{
    14, 10, 18, 8, 12, 16, 10, 14, 20, 8,
    12, 18, 10, 16, 14, 8, 12, 20, 10, 16,
    14, 8, 18, 12, 10
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
    255, 240, 228, 216, 204, 192, 180, 168, 156, 144,
    132, 120, 108, 96, 84, 74, 64, 56, 50, 44
};

constexpr uint8_t MAX_TRAIL_LENGTH =
    static_cast<uint8_t>(sizeof(TRAIL_BRIGHTNESS) / sizeof(TRAIL_BRIGHTNESS[0]));
constexpr uint16_t Q8_ONE = 256U;
constexpr uint8_t REFERENCE_FRAME_TICKS = 10U;

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
        state.head_y_q8 = static_cast<int16_t>(INITIAL_HEAD[column] *
                                               static_cast<int16_t>(Q8_ONE));
        const uint16_t velocity_divisor = static_cast<uint16_t>(
            REFERENCE_FRAME_TICKS * PERIOD_TICKS[column]);
        state.velocity_q8 = static_cast<uint8_t>(
            (Q8_ONE + velocity_divisor / 2U) / velocity_divisor);
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
    for (uint8_t column = 0U; column < LED_WIDTH; ++column)
    {
        ColumnState& state = _columns[column];
        state.head_y_q8 = static_cast<int16_t>(state.head_y_q8 + state.velocity_q8);
        const int16_t reset_position_q8 = static_cast<int16_t>(
            (LED_HEIGHT + state.trail_length) * Q8_ONE);
        if (state.head_y_q8 >= reset_position_q8)
        {
            state.head_y_q8 = static_cast<int16_t>(
                -static_cast<int16_t>(state.reset_delay * Q8_ONE));
        }
    }

    return true;
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
        const int16_t distance_q8 = static_cast<int16_t>(
            state.head_y_q8 - static_cast<int16_t>(line * Q8_ONE));
        const int16_t trail_extent_q8 = static_cast<int16_t>(state.trail_length * Q8_ONE);
        if ((distance_q8 <= -static_cast<int16_t>(Q8_ONE)) ||
            (distance_q8 >= trail_extent_q8))
        {
            continue;
        }

        uint16_t trail_brightness;
        if (distance_q8 < 0)
        {
            const uint16_t leading_weight = static_cast<uint16_t>(Q8_ONE + distance_q8);
            trail_brightness = static_cast<uint16_t>(
                (static_cast<uint32_t>(TRAIL_BRIGHTNESS[0]) * leading_weight + 128U) >> 8U);
        }
        else
        {
            const uint8_t trail_index = static_cast<uint8_t>(distance_q8 >> 8U);
            const uint8_t fraction = static_cast<uint8_t>(distance_q8 & 0xFF);
            const uint8_t current = TRAIL_BRIGHTNESS[trail_index];
            const uint8_t next = (trail_index + 1U < state.trail_length) ?
                                 TRAIL_BRIGHTNESS[trail_index + 1U] : 0U;
            trail_brightness = static_cast<uint16_t>(
                (static_cast<uint32_t>(current) * (Q8_ONE - fraction) +
                 static_cast<uint32_t>(next) * fraction + 128U) >> 8U);
        }

        const uint32_t brightness =
            static_cast<uint32_t>(trail_brightness) * state.peak_brightness;
        output[column] = static_cast<uint8_t>((brightness + 127U) / 255U);
    }
}

} // namespace gfx::animations
