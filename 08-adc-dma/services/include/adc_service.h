#ifndef ADC_SERVICE_H
#define ADC_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "adc_types.h"

void adc_service_init(void);
void adc_service_process(void);

bool adc_service_take_measurement(adc_measurement_t *measurement);
uint32_t adc_service_get_dma_overrun_count(void);
uint32_t adc_service_get_dma_error_count(void);

#endif
