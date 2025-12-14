#include "task.h"

Task mytask;

void setup()
{
    mytask.init();
}

void loop()
{
    mytask.tick();
}
