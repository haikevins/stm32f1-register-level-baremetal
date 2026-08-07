#include "application.h"

#include <stdbool.h>

#include "adc_service.h"
#include "adc_types.h"
#include "application_config.h"
#include "indication_service.h"

volatile uint16_t application_adc_average_raw;
volatile uint16_t application_adc_minimum_raw;
volatile uint16_t application_adc_maximum_raw;
volatile uint16_t application_adc_millivolts;
volatile uint32_t application_adc_sequence;
volatile uint32_t application_adc_dma_overruns;
volatile uint32_t application_adc_dma_errors;

static bool g_led_active;

void application_init(void)
{
    application_adc_average_raw = 0U;
    application_adc_minimum_raw = 0U;
    application_adc_maximum_raw = 0U;
    application_adc_millivolts = 0U;
    application_adc_sequence = 0U;
    application_adc_dma_overruns = 0U;
    application_adc_dma_errors = 0U;

    g_led_active = false;
    indication_service_set(INDICATION_STATUS, false);
}

void application_process(void)
{
    adc_measurement_t measurement;

    adc_service_process();

    application_adc_dma_overruns =
        adc_service_get_dma_overrun_count();
    application_adc_dma_errors =
        adc_service_get_dma_error_count();

    if (!adc_service_take_measurement(&measurement))
    {
        return;
    }

    application_adc_average_raw = measurement.average_raw;
    application_adc_minimum_raw = measurement.minimum_raw;
    application_adc_maximum_raw = measurement.maximum_raw;
    application_adc_millivolts = measurement.millivolts;
    application_adc_sequence = measurement.sequence;

    if ((!g_led_active) &&
        (measurement.millivolts >=
         APPLICATION_ADC_LED_ON_THRESHOLD_MV))
    {
        g_led_active = true;
        indication_service_set(INDICATION_STATUS, true);
    }
    else if (g_led_active &&
             (measurement.millivolts <=
              APPLICATION_ADC_LED_OFF_THRESHOLD_MV))
    {
        g_led_active = false;
        indication_service_set(INDICATION_STATUS, false);
    }
}
