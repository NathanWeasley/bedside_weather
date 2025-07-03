#pragma once

#include <stdint.h>

#define LED_HEIGHT          (16)
#define LED_WIDTH           (25)
#define LED_WIDTH_BYTE      ((LED_WIDTH/8) + ((LED_WIDTH%8) == 0 ? 0 : 1))
