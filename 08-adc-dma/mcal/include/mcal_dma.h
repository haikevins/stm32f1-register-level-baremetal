#ifndef MCAL_DMA_H
#define MCAL_DMA_H

#include <stdbool.h>
#include <stdint.h>

#define MCAL_DMA_EVENT_HALF_TRANSFER      (UINT32_C(1) << 0U)
#define MCAL_DMA_EVENT_TRANSFER_COMPLETE  (UINT32_C(1) << 1U)
#define MCAL_DMA_EVENT_TRANSFER_ERROR     (UINT32_C(1) << 2U)

bool mcal_dma_adc1_channel1_configure(
    volatile uint16_t *memory,
    uint16_t sample_count);

void mcal_dma_adc1_channel1_start(void);
void mcal_dma_adc1_channel1_stop(void);
uint32_t mcal_dma_adc1_channel1_take_irq_events(void);

#endif
