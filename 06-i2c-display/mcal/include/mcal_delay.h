#ifndef MCAL_DELAY_H
#define MCAL_DELAY_H

#include <stdint.h>

void mcal_delay_busy_ms(uint32_t core_clock_hz, uint32_t delay_ms);

#endif
