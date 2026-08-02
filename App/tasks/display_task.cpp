#include "tasks/display_task.h"
#include "tasks/comm_task.h"
#include "graphics/gfx_animated.hpp"
#include "graphics/gfx_paint.hpp"
#include "graphics/gfx_api.h"
#include "graphics/font_5.h"

using namespace gfx;

// Define high-level monochrome video memory
static uint8_t g_vram[LED_CNT];

// Declare total canvas
using Cvs = Canvas<500, 16>;

// Declare display window and mask for weather icon
using IconWnd = Window<Cvs::width(), Cvs::height(), 0, 0, 48, 16>;
// using IconMask = Mask<16, 16, AnimateMethod::METHOD_LSHIFT, 4, IconWnd>;
using IconMask = Mask<25, 16, AnimateMethod::METHOD_LSHIFT, 1, IconWnd>;

// Declare display window and mask for temperature
using TempWnd = Window<Cvs::width(), Cvs::height(), 16, 0, 9, 16>;
using TempMask = Mask<9, 16, AnimateMethod::METHOD_NOANIMATION, 1, TempWnd>;

// Declare physical display
using Screen = Display<LED_WIDTH, LED_HEIGHT, g_vram>;
using IconZone = DisplayZone<IconMask, Screen>;
using TempZone = DisplayZone<TempMask, Screen>;

static IconMask g_icon_mask;
static IconZone g_icon_zone;
[[maybe_unused]] static TempMask g_temp_mask;
[[maybe_unused]] static TempZone g_temp_zone;


RxData DisplayTask::_rxdata = { "Apple" };
DisplayInfo DisplayTask::_display_info = {};

void DisplayTask::init()
{
    CommTask::instance()->register_callback(5, DisplayTask::message_callback);

    set_whole<IconWnd>(0);
    // set_whole<TempWnd>(128);
    
    // draw_string<Wnd1>(0, 0, "A quick brown fox jumps over the lazy dog.", font5_table, 255, 0);
    // draw_string<Wnd2>(0, 0, "0123456789!@#$%^&*()-=_+[]{};':\",.<>/?\\|", font5_table, 255);
}

void DisplayTask::tick()
{
    /* 当前仍保留 hello/RxData 显示测试；正式 UI 可直接消费 _display_info。 */
    // Content update
    draw_string<IconWnd>(0, 0, _rxdata.str, font5_table, 255, 0);
    
    // Tick all zones
    // tick_all(zone1, mask1, zone2, mask2);
    g_icon_zone.tick_then_update(g_icon_mask);
    // g_temp_zone.tick_then_update(g_temp_mask);

    // Repaint
    gfx_update_img(Screen::data());
}

void DisplayTask::set_display_info(const DisplayInfo& info)
{
    _display_info = info;
}

void DisplayTask::message_callback(const v1::Packet& pk)
{
    if (!pk.unpack(_rxdata))
    {
        MX_USART1_UART_DMASend((const uint8_t *)"Data recv error.\n", 17);
    }
    else
    {
        /* 测试载荷固定为“hello”，仍强制保留C字符串结尾。 */
        _rxdata.str[sizeof(_rxdata.str) - 1U] = '\0';
    }
}
