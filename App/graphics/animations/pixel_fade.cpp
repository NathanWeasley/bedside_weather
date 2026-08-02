#include "graphics/animations/pixel_fade.hpp"

namespace gfx::animations
{
namespace
{

constexpr uint32_t PIXEL_PRESET_SEED = 0x1625A5C3UL;
constexpr uint8_t MIN_FADE_SPEED = 6U;
constexpr uint8_t MAX_FADE_SPEED = 24U;
constexpr uint16_t Q8_ONE = 256U;
constexpr uint16_t Q8_MAX_BRIGHTNESS = 255U * Q8_ONE;
constexpr uint8_t REFERENCE_FRAME_TICKS = 10U;

struct PixelPreset
{
    uint8_t brightness;
    int8_t velocity;
};

constexpr uint32_t xorshift32(uint32_t value)
{
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    return value;
}

/*
 * 固定种子保证固件可复现；修改种子即可在下一次编译时生成另一组像素参数。
 * 预设常量位于 Flash，不消耗运行时随机状态。
 */
struct PixelPresetTable
{
    PixelPreset values[LED_CNT];

    constexpr PixelPresetTable()
    : values{}
    {
        uint32_t random = PIXEL_PRESET_SEED;
        for (uint16_t pixel = 0U; pixel < LED_CNT; ++pixel)
        {
            random = xorshift32(random);
            values[pixel].brightness = static_cast<uint8_t>(random & 0xFFU);

            random = xorshift32(random);
            const uint8_t speed = static_cast<uint8_t>(
                MIN_FADE_SPEED + (random % (MAX_FADE_SPEED - MIN_FADE_SPEED + 1U)));
            values[pixel].velocity = ((random & 0x100U) != 0U) ?
                static_cast<int8_t>(speed) : static_cast<int8_t>(-static_cast<int8_t>(speed));
        }
    }
};

constexpr PixelPresetTable PIXEL_PRESETS{};

} // namespace

PixelFadeView::PixelFadeView()
{
    reset();
}

void PixelFadeView::reset()
{
    for (uint16_t pixel = 0U; pixel < LED_CNT; ++pixel)
    {
        const PixelPreset& preset = PIXEL_PRESETS.values[pixel];
        PixelState& state = _pixels[pixel];
        state.brightness_q8 = static_cast<uint16_t>(preset.brightness * Q8_ONE);

        const uint16_t reference_speed = static_cast<uint16_t>(
            (preset.velocity < 0) ? -preset.velocity : preset.velocity);
        const int16_t velocity_q8 = static_cast<int16_t>(
            (reference_speed * Q8_ONE + REFERENCE_FRAME_TICKS / 2U) /
            REFERENCE_FRAME_TICKS);
        state.velocity_q8 = (preset.velocity < 0) ?
                            static_cast<int16_t>(-velocity_q8) : velocity_q8;
        state.quantization_error = static_cast<uint8_t>(
            pixel * 73U + preset.brightness);
        state.output_brightness = preset.brightness;
    }
}

bool PixelFadeView::tick()
{
    for (uint16_t pixel = 0U; pixel < LED_CNT; ++pixel)
    {
        PixelState& state = _pixels[pixel];
        const int32_t next = static_cast<int32_t>(state.brightness_q8) + state.velocity_q8;

        if (next >= Q8_MAX_BRIGHTNESS)
        {
            state.brightness_q8 = Q8_MAX_BRIGHTNESS;
            state.quantization_error = 0U;
            state.output_brightness = 255U;
            if (state.velocity_q8 > 0)
            {
                state.velocity_q8 = static_cast<int16_t>(-state.velocity_q8);
            }
        }
        else if (next <= 0)
        {
            state.brightness_q8 = 0U;
            state.quantization_error = 0U;
            state.output_brightness = 0U;
            if (state.velocity_q8 < 0)
            {
                state.velocity_q8 = static_cast<int16_t>(-state.velocity_q8);
            }
        }
        else
        {
            state.brightness_q8 = static_cast<uint16_t>(next);

            /*
             * 用误差累积在相邻帧间交替输出整数亮度，使 Q8.8 的小数部分
             * 转化为时间域占空比，避免直接取整造成明显的灰度跳变。
             */
            uint16_t output = static_cast<uint16_t>(state.brightness_q8 >> 8U);
            uint16_t error = static_cast<uint16_t>(
                state.quantization_error + (state.brightness_q8 & 0xFFU));
            if (error >= Q8_ONE)
            {
                ++output;
                error -= Q8_ONE;
            }
            state.quantization_error = static_cast<uint8_t>(error);
            state.output_brightness = static_cast<uint8_t>(output);
        }
    }

    return true;
}

void PixelFadeView::get_line(uint16_t line, uint8_t * output) const
{
    if ((line >= LED_HEIGHT) || (output == nullptr))
    {
        return;
    }

    const uint16_t first_pixel = static_cast<uint16_t>(line * LED_WIDTH);
    for (uint8_t column = 0U; column < LED_WIDTH; ++column)
    {
        output[column] = _pixels[first_pixel + column].output_brightness;
    }
}

} // namespace gfx::animations
