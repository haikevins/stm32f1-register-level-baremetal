#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdint.h>

extern volatile uint16_t application_adc_average_raw;
extern volatile uint16_t application_adc_minimum_raw;
extern volatile uint16_t application_adc_maximum_raw;
extern volatile uint16_t application_adc_millivolts;
extern volatile uint32_t application_adc_sequence;
extern volatile uint32_t application_adc_dma_overruns;
extern volatile uint32_t application_adc_dma_errors;

void application_init(void);
void application_process(void);

#endif
