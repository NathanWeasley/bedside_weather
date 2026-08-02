#pragma once

#include "config.h"

#include <cstdint>

namespace gfx::animations
{

/* 每个像素独立在 0~255 之间往返渐变的过程式全屏视图。 */
class PixelFadeView
{
    struct PixelState
    {
        uint16_t brightness_q8;
        int16_t velocity_q8;
        uint8_t quantization_error;
        uint8_t output_brightness;
    };

    PixelState _pixels[LED_CNT];

public:
    PixelFadeView();

    void reset();
    bool tick();
    void get_line(uint16_t line, uint8_t * output) const;

    static constexpr uint16_t x() { return 0U; }
    static constexpr uint16_t y() { return 0U; }
    static constexpr uint16_t width() { return LED_WIDTH; }
    static constexpr uint16_t height() { return LED_HEIGHT; }
};

} // namespace gfx::animations
