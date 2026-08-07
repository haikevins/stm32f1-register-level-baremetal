#ifndef BOARD_ADC_DMA_H
#define BOARD_ADC_DMA_H

#include <stdbool.h>
#include <stdint.h>

bool board_adc_dma_init(uint32_t apb2_clock_hz,
                        uint32_t timer_clock_hz);

bool board_adc_dma_take_sample_block(uint16_t *samples,
                                     uint16_t capacity,
                                     uint16_t *sample_count);

uint32_t board_adc_dma_get_overrun_count(void);
uint32_t board_adc_dma_get_error_count(void);

#endif
