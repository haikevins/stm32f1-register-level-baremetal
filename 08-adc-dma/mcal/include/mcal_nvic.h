#ifndef MCAL_NVIC_H
#define MCAL_NVIC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MCAL_NVIC_IRQ_DMA1_CHANNEL1 = 0
} mcal_nvic_irq_t;

bool mcal_nvic_enable_irq(mcal_nvic_irq_t irq,
                          uint8_t priority);
void mcal_nvic_disable_irq(mcal_nvic_irq_t irq);

#endif
