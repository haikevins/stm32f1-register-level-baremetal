#include "adc_service.h"

#include <stddef.h>

#include "board_adc_dma.h"
#include "board_config.h"

static uint16_t
    g_samples[BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT];

static adc_measurement_t g_latest_measurement;
static bool g_measurement_pending;

void adc_service_init(void)
{
    g_latest_measurement.average_raw = 0U;
    g_latest_measurement.minimum_raw = 0U;
    g_latest_measurement.maximum_raw = 0U;
    g_latest_measurement.millivolts = 0U;
    g_latest_measurement.sequence = 0U;
    g_measurement_pending = false;
}

void adc_service_process(void)
{
    uint16_t sample_count;
    uint16_t index;
    uint16_t minimum;
    uint16_t maximum;
    uint16_t average;
    uint32_t sum = 0U;

    if (!board_adc_dma_take_sample_block(
            g_samples,
            BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT,
            &sample_count))
    {
        return;
    }

    if (sample_count == 0U)
    {
        return;
    }

    minimum = g_samples[0];
    maximum = g_samples[0];

    for (index = 0U; index < sample_count; index++)
    {
        const uint16_t sample = g_samples[index];

        sum += sample;

        if (sample < minimum)
        {
            minimum = sample;
        }

        if (sample > maximum)
        {
            maximum = sample;
        }
    }

    average = (uint16_t)(
        (sum + ((uint32_t)sample_count / 2U)) /
        (uint32_t)sample_count);

    g_latest_measurement.average_raw = average;
    g_latest_measurement.minimum_raw = minimum;
    g_latest_measurement.maximum_raw = maximum;
    g_latest_measurement.millivolts = (uint16_t)(
        (((uint32_t)average * BOARD_ADC_REFERENCE_MV) +
         (BOARD_ADC_MAX_RAW_VALUE / 2U)) /
        BOARD_ADC_MAX_RAW_VALUE);
    g_latest_measurement.sequence++;

    g_measurement_pending = true;
}

bool adc_service_take_measurement(adc_measurement_t *measurement)
{
    if ((measurement == NULL) || !g_measurement_pending)
    {
        return false;
    }

    *measurement = g_latest_measurement;
    g_measurement_pending = false;

    return true;
}

uint32_t adc_service_get_dma_overrun_count(void)
{
    return board_adc_dma_get_overrun_count();
}

uint32_t adc_service_get_dma_error_count(void)
{
    return board_adc_dma_get_error_count();
}
