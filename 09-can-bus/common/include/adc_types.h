#ifndef ADC_TYPES_H
#define ADC_TYPES_H

#include <stdint.h>

typedef struct
{
    uint16_t average_raw;
    uint16_t minimum_raw;
    uint16_t maximum_raw;
    uint16_t millivolts;
    uint32_t sequence;
} adc_measurement_t;

#endif
