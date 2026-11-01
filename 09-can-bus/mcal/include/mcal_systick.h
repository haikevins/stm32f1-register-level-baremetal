#ifndef MCAL_SYSTICK_H
#define MCAL_SYSTICK_H

#include <stdbool.h>
#include <stdint.h>

bool mcal_systick_init(uint32_t core_clock_hz, uint32_t tick_hz);
void mcal_systick_deinit(void);
uint32_t mcal_systick_get_ticks(void);

#endif
