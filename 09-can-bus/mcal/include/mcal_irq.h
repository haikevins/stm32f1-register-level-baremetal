#ifndef MCAL_IRQ_H
#define MCAL_IRQ_H

#include <stdint.h>

uint32_t mcal_irq_save_and_disable(void);
void mcal_irq_restore(uint32_t saved_primask);

#endif
