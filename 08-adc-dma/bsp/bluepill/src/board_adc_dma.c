#include "board_adc_dma.h"

#include <stddef.h>

#include "board_config.h"
#include "board_pins.h"
#include "mcal_irq.h"
#include "mcal_adc.h"
#include "mcal_dma.h"
#include "mcal_gpio.h"
#include "mcal_nvic.h"
#include "mcal_timer_trigger.h"

static volatile uint16_t
    g_dma_buffer[BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT];

static uint16_t
    g_completed_block[BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT];

static volatile bool g_block_ready;
static volatile uint32_t g_overrun_count;
static volatile uint32_t g_error_count;

static void publish_block(uint16_t offset)
{
    uint16_t index;

    if (g_block_ready)
    {
        g_overrun_count++;
    }

    for (index = 0U;
         index < BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT;
         index++)
    {
        g_completed_block[index] =
            g_dma_buffer[(uint16_t)(offset + index)];
    }

    g_block_ready = true;
}

bool board_adc_dma_init(uint32_t apb2_clock_hz,
                        uint32_t timer_clock_hz)
{
    g_block_ready = false;
    g_overrun_count = 0U;
    g_error_count = 0U;

    if (!mcal_gpio_configure(BOARD_ADC_INPUT_PORT,
                             BOARD_ADC_INPUT_PIN,
                             MCAL_GPIO_MODE_INPUT_ANALOG,
                             MCAL_GPIO_LEVEL_LOW))
    {
        return false;
    }

    if (!mcal_dma_adc1_channel1_configure(
            g_dma_buffer,
            BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT))
    {
        return false;
    }

    if (!mcal_timer3_trigger_init(
            timer_clock_hz,
            BOARD_ADC_TRIGGER_TIMER_TICK_HZ,
            BOARD_ADC_SAMPLE_RATE_HZ))
    {
        return false;
    }

    if (!mcal_adc1_init_regular_channel(
            0U,
            apb2_clock_hz,
            BOARD_ADC_MAX_CLOCK_HZ,
            MCAL_ADC_TRIGGER_TIM3_TRGO,
            BOARD_ADC_CALIBRATION_TIMEOUT_ITERATIONS))
    {
        return false;
    }

    if (!mcal_nvic_enable_irq(MCAL_NVIC_IRQ_DMA1_CHANNEL1,
                              BOARD_ADC_DMA_IRQ_PRIORITY))
    {
        return false;
    }

    mcal_dma_adc1_channel1_start();
    mcal_timer3_trigger_start();

    return true;
}

bool board_adc_dma_take_sample_block(uint16_t *samples,
                                     uint16_t capacity,
                                     uint16_t *sample_count)
{
    uint32_t saved_primask;
    uint16_t index;

    if ((samples == NULL) ||
        (sample_count == NULL) ||
        (capacity < BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT))
    {
        return false;
    }

    saved_primask = mcal_irq_save_and_disable();

    if (!g_block_ready)
    {
        mcal_irq_restore(saved_primask);
        return false;
    }

    for (index = 0U;
         index < BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT;
         index++)
    {
        samples[index] = g_completed_block[index];
    }

    g_block_ready = false;
    *sample_count = BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT;

    mcal_irq_restore(saved_primask);

    return true;
}

uint32_t board_adc_dma_get_overrun_count(void)
{
    return g_overrun_count;
}

uint32_t board_adc_dma_get_error_count(void)
{
    return g_error_count;
}

void DMA1_Channel1_IRQHandler(void)
{
    const uint32_t events =
        mcal_dma_adc1_channel1_take_irq_events();

    if ((events & MCAL_DMA_EVENT_TRANSFER_ERROR) != 0U)
    {
        g_error_count++;
    }

    if ((events & MCAL_DMA_EVENT_HALF_TRANSFER) != 0U)
    {
        publish_block(0U);
    }

    if ((events & MCAL_DMA_EVENT_TRANSFER_COMPLETE) != 0U)
    {
        publish_block(BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT);
    }
}
