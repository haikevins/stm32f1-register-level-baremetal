#ifndef MCAL_ADC_H
#define MCAL_ADC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MCAL_ADC_TRIGGER_TIM3_TRGO = 0
} mcal_adc_trigger_t;

bool mcal_adc1_init_regular_channel(
    uint8_t channel,
    uint32_t apb2_clock_hz,
    uint32_t max_adc_clock_hz,
    mcal_adc_trigger_t trigger,
    uint32_t calibration_timeout_iterations);

uintptr_t mcal_adc1_data_register_address(void);

#endif
