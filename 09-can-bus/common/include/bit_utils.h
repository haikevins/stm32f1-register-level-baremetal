#ifndef BIT_UTILS_H
#define BIT_UTILS_H

#include <stdint.h>

#define BIT_U32(position) (UINT32_C(1) << (position))
#define FIELD_MASK_U32(width, position) \
    ((((UINT32_C(1) << (width)) - UINT32_C(1))) << (position))

#endif
