#include "tasks/test_task.h"
#include "graphics/gfx_animated.hpp"

#define UNUSED(x) (void)(x)

using namespace gfx;

static uint8_t g_vram[LED_CNT];

using DrawZone = Canvas<32, 16>;
using IconMask = Mask<16, 16, METHOD_LSHIFT, 10, DrawZone>;

using Screen = Display<LED_WIDTH, LED_HEIGHT, g_vram>;
using IconZone = DisplayZone<IconMask, 0, 0, Screen>;



extern "C" void test_task_init(task_param_t * param)
{
    UNUSED(param);
}

extern "C" void test_task_tick()
{
    ;
}

