#ifndef MCAL_TIMER_H
#define MCAL_TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MCAL_TIMER_INSTANCE_2 = 0,
    MCAL_TIMER_INSTANCE_COUNT
} mcal_timer_instance_t;

typedef enum
{
    MCAL_TIMER_CHANNEL_1 = 0
} mcal_timer_channel_t;

bool mcal_timer_pwm_init(mcal_timer_instance_t instance,
                         mcal_timer_channel_t channel,
                         uint32_t timer_clock_hz,
                         uint32_t timer_tick_hz,
                         uint32_t pwm_frequency_hz,
                         uint16_t initial_duty_permille);

void mcal_timer_pwm_set_duty_permille(mcal_timer_instance_t instance,
                                      mcal_timer_channel_t channel,
                                      uint16_t duty_permille);

#endif
