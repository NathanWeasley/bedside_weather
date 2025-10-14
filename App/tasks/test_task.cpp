#include "tasks/test_task.h"
#include "graphics/gfx_animated.hpp"
#include "graphics/gfx_paint.hpp"

#define UNUSED(x) (void)(x)

using namespace gfx;

static uint8_t g_vram[LED_CNT];

using Cvs = Canvas<16, 200>;
using Wnd1 = Window<Cvs::__cwidth, Cvs::__cheight, 0, 0, 200, 8>;
using Wnd2 = Window<Cvs::__cwidth, Cvs::__cheight, 0, 8, 200, 8>;
using IconMask = Mask<16, 8, METHOD_LSHIFT, 10, Wnd1>;

using Screen = Display<LED_WIDTH, LED_HEIGHT, g_vram>;
using IconZone = DisplayZone<IconMask, 0, 0, Screen>;



extern "C" uint32_t test_task_param_size()
{
    return 1;
}

extern "C" void test_task_init(task_param_t * param)
{
    UNUSED(param);

    set_whole<Wnd1>(0);
    set_whole<Wnd2>(128);
    
}

extern "C" void test_task_tick()
{
    // Repaint


    // Tick all zones
}

