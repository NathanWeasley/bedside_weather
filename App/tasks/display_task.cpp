#include "tasks/display_task.h"

#include "graphics/gfx_animated.hpp"
#include "graphics/gfx_api.h"
#include "graphics/gfx_paint.hpp"
#include "graphics/font_5.h"

#include <cstring>

using namespace gfx;

namespace
{

constexpr uint16_t LINE_CANVAS_WIDTH = 256U;
constexpr uint16_t LINE_HEIGHT = 5U;
constexpr uint16_t SCROLL_GAP_PIXELS = 8U;
constexpr uint16_t SCROLL_PERIOD_TICKS = 1U;   // DisplayTask 为 100 ms，故每 100 ms 移动 1 像素

struct GuiCanvasTag;
using GuiCanvas = Canvas<LINE_CANVAS_WIDTH, LINE_HEIGHT * DISPLAY_LINE_COUNT, GuiCanvasTag>;

using NetworkWindow = Window<GuiCanvas, 0U, 0U, LINE_CANVAS_WIDTH, LINE_HEIGHT>;
using DateTimeWindow = Window<GuiCanvas, 0U, 5U, LINE_CANVAS_WIDTH, LINE_HEIGHT>;
using WeatherWindow = Window<GuiCanvas, 0U, 10U, LINE_CANVAS_WIDTH, LINE_HEIGHT>;

using NetworkViewport = Viewport<LED_WIDTH, LINE_HEIGHT, NetworkWindow>;
using DateTimeViewport = Viewport<LED_WIDTH, LINE_HEIGHT, DateTimeWindow>;
using WeatherViewport = Viewport<LED_WIDTH, LINE_HEIGHT, WeatherWindow>;

using NetworkView = AnimatedView<NetworkViewport, ScrollAnimator>;
using DateTimeView = AnimatedView<DateTimeViewport, ScrollAnimator>;
using WeatherView = AnimatedView<WeatherViewport, ScrollAnimator>;

static uint8_t g_vram[LED_CNT];
using Screen = Display<LED_WIDTH, LED_HEIGHT, g_vram>;

using NetworkZone = DisplayZone<NetworkView, Screen>;
using DateTimeZone = DisplayZone<DateTimeView, Screen>;
using WeatherZone = DisplayZone<WeatherView, Screen>;

static NetworkView g_network_view;
static DateTimeView g_datetime_view;
static WeatherView g_weather_view;
static NetworkZone g_network_zone;
static DateTimeZone g_datetime_zone;
static WeatherZone g_weather_zone;

template <typename WindowType, typename ViewType, typename ZoneType>
void render_line(const char * text, ViewType& view, ZoneType& zone)
{
    set_whole<WindowType>(0U);
    const uint16_t text_width = draw_string<WindowType>(
        0U, 0U, text, font5_table, 255U, 0);

    const bool scrolling = text_width > LED_WIDTH;
    uint16_t cycle_width = LED_WIDTH;
    if (scrolling)
    {
        const uint32_t requested_width = static_cast<uint32_t>(text_width) + SCROLL_GAP_PIXELS;
        cycle_width = (requested_width > WindowType::width()) ?
                      WindowType::width() : static_cast<uint16_t>(requested_width);
    }

    view.viewport().set_cycle_size(cycle_width, LINE_HEIGHT);
    view.animator().configure(
        ScrollDirection::LEFT,
        SCROLL_PERIOD_TICKS,
        0U,
        scrolling);
    view.reset();
    zone.update(view);
}

} // namespace

void DisplayTask::init()
{
    set_whole<GuiCanvas>(0U);
    memset(g_vram, 0, sizeof(g_vram));
    _dirty_lines = (1U << DISPLAY_LINE_COUNT) - 1U;
}

void DisplayTask::tick()
{
    bool updated = false;

    if ((_dirty_lines & (1U << 0)) != 0U)
    {
        render_line<NetworkWindow>(_content.lines[0], g_network_view, g_network_zone);
        updated = true;
    }
    if ((_dirty_lines & (1U << 1)) != 0U)
    {
        render_line<DateTimeWindow>(_content.lines[1], g_datetime_view, g_datetime_zone);
        updated = true;
    }
    if ((_dirty_lines & (1U << 2)) != 0U)
    {
        render_line<WeatherWindow>(_content.lines[2], g_weather_view, g_weather_zone);
        updated = true;
    }
    _dirty_lines = 0U;

    updated = tick_all(
        g_network_zone, g_network_view,
        g_datetime_zone, g_datetime_view,
        g_weather_zone, g_weather_view) || updated;

    if (updated)
    {
        gfx_update_img(Screen::data());
    }
}

void DisplayTask::set_display_content(const DisplayContent& content)
{
    for (uint8_t line = 0U; line < DISPLAY_LINE_COUNT; ++line)
    {
        if (memcmp(_content.lines[line],
                   content.lines[line],
                   DISPLAY_LINE_TEXT_CAPACITY) != 0)
        {
            memcpy(_content.lines[line],
                   content.lines[line],
                   DISPLAY_LINE_TEXT_CAPACITY);
            _content.lines[line][DISPLAY_LINE_TEXT_CAPACITY - 1U] = '\0';
            _dirty_lines |= static_cast<uint8_t>(1U << line);
        }
    }
}
