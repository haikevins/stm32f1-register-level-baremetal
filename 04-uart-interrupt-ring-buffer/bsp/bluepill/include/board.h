#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

typedef enum
{
    BOARD_CLOCK_SOURCE_HSE_PLL = 0,
    BOARD_CLOCK_SOURCE_HSI_FALLBACK
} board_clock_source_t;

bool board_init(void);
board_clock_source_t board_get_clock_source(void);

#endif
