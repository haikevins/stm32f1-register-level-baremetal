#ifndef BOARD_TIMEBASE_H
#define BOARD_TIMEBASE_H

#include <stdbool.h>
#include <stdint.h>

bool board_timebase_init(uint32_t core_clock_hz);
uint32_t board_timebase_now_ms(void);

#endif
