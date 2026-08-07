#ifndef MCAL_TIMER_TRIGGER_H
#define MCAL_TIMER_TRIGGER_H

#include <stdbool.h>
#include <stdint.h>

bool mcal_timer3_trigger_init(uint32_t timer_clock_hz,
                              uint32_t timer_tick_hz,
                              uint32_t trigger_frequency_hz);
void mcal_timer3_trigger_start(void);
void mcal_timer3_trigger_stop(void);

#endif
