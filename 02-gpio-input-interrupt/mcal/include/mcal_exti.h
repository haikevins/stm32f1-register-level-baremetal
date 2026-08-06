#ifndef MCAL_EXTI_H
#define MCAL_EXTI_H

#include <stdbool.h>
#include <stdint.h>

#include "mcal_gpio.h"

typedef enum
{
    MCAL_EXTI_TRIGGER_RISING = 0,
    MCAL_EXTI_TRIGGER_FALLING,
    MCAL_EXTI_TRIGGER_BOTH
} mcal_exti_trigger_t;

bool mcal_exti_configure_line(uint8_t line,
                              mcal_gpio_port_t port,
                              mcal_exti_trigger_t trigger,
                              uint8_t irq_priority);
void mcal_exti_disable_line(uint8_t line);
bool mcal_exti_take_event(uint8_t line);

#endif
