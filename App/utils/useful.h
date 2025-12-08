#pragma once

#include <stdint.h>

#define UNUSED(x) (void)(x)

#ifdef __cplusplus
extern "C"
{
#endif

inline bool is_big_endian(void)
{
    union
    {
        uint32_t i;
        char c[4];
    } bint = { 0x01020304 };

    return bint.c[0] == 1;
}

#ifdef __cplusplus
}
#endif
