#include "tasks/test_task.h"
#include "graphics/gfx_animated.hpp"
#include "graphics/gfx_paint.hpp"
#include "graphics/gfx_api.h"
#include "graphics/font_5.h"

#define UNUSED(x) (void)(x)

using namespace gfx;

static uint8_t g_vram[LED_CNT];

using Cvs = Canvas<200, 16>;
using Wnd1 = Window<Cvs::width(), Cvs::height(), 0, 0, 200, 8>;
using Wnd2 = Window<Cvs::width(), Cvs::height(), 0, 8, 200, 8>;
using IconMask1 = Mask<25, 8, METHOD_LSHIFT, 10, Wnd1>;
using IconMask2 = Mask<25, 8, METHOD_LSHIFT, 20, Wnd2>;

using Screen = Display<LED_WIDTH, LED_HEIGHT, g_vram>;
using IconZone1 = DisplayZone<IconMask1, Screen>;
using IconZone2 = DisplayZone<IconMask2, Screen>;

static IconMask1 mask1(0, 0);
static IconMask2 mask2(0, 0);
static IconZone1 zone1;
static IconZone2 zone2;

static task_param_t test_task_param;

extern "C" uint32_t test_task_param_size()
{
    return 1;
}

extern "C" void test_task_init(task_param_t * param)
{
    if (param)
    {
        test_task_param = *param;
    }

    set_whole<Wnd1>(0);
    set_whole<Wnd2>(128);
    
    // draw_char<Wnd1>(0, 0, 'A', font5_table, 255, 0);
    draw_string<Wnd1>(0, 0, "A quick brown fox jumps over the lazy dog.", font5_table, 255, 0);
    draw_string<Wnd2>(0, 0, "0123456789!@#$%^&*()-=_+[]{};':\",.<>/?\\|", font5_table, 255);

    // gfx_update_img(g_test_img);

    // zone1.update(mask1);
    // zone2.update(mask2);
    // gfx_update_img(Screen::data());
}

extern "C" void test_task_tick()
{
    // Redraw windows


    // Tick all zones
    // tick_all(zone1, mask1, zone2, mask2);
    zone1.tick_then_update(mask1);
    zone2.tick_then_update(mask2);

    // Repaint
    gfx_update_img(Screen::data());
}

