#include "mcal_dma.h"

#include <stddef.h>
#include <stdint.h>

#include "mcal_adc.h"
#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

bool mcal_dma_adc1_channel1_configure(
    volatile uint16_t *memory,
    uint16_t sample_count)
{
    if ((memory == NULL) || (sample_count == 0U))
    {
        return false;
    }

    STM32_RCC->AHBENR |= STM32_RCC_AHBENR_DMA1EN;
    (void)STM32_RCC->AHBENR;

    STM32_DMA1_CHANNEL1->CCR &= ~STM32_DMA_CCR_EN;

    STM32_DMA1->IFCR =
        STM32_DMA_IFCR_CGIF1 |
        STM32_DMA_IFCR_CTCIF1 |
        STM32_DMA_IFCR_CHTIF1 |
        STM32_DMA_IFCR_CTEIF1;

    STM32_DMA1_CHANNEL1->CNDTR = sample_count;
    STM32_DMA1_CHANNEL1->CPAR =
        (uint32_t)mcal_adc1_data_register_address();
    STM32_DMA1_CHANNEL1->CMAR =
        (uint32_t)(uintptr_t)memory;

    STM32_DMA1_CHANNEL1->CCR =
        STM32_DMA_CCR_TCIE |
        STM32_DMA_CCR_HTIE |
        STM32_DMA_CCR_TEIE |
        STM32_DMA_CCR_CIRC |
        STM32_DMA_CCR_MINC |
        STM32_DMA_CCR_PSIZE_16BIT |
        STM32_DMA_CCR_MSIZE_16BIT |
        STM32_DMA_CCR_PL_HIGH;

    return true;
}

void mcal_dma_adc1_channel1_start(void)
{
    STM32_DMA1_CHANNEL1->CCR |= STM32_DMA_CCR_EN;
}

void mcal_dma_adc1_channel1_stop(void)
{
    STM32_DMA1_CHANNEL1->CCR &= ~STM32_DMA_CCR_EN;
}

uint32_t mcal_dma_adc1_channel1_take_irq_events(void)
{
    const uint32_t status = STM32_DMA1->ISR;
    uint32_t events = 0U;
    uint32_t clear_mask = 0U;

    if ((status & STM32_DMA_ISR_TEIF1) != 0U)
    {
        events |= MCAL_DMA_EVENT_TRANSFER_ERROR;
        clear_mask |= STM32_DMA_IFCR_CTEIF1;
    }

    if ((status & STM32_DMA_ISR_HTIF1) != 0U)
    {
        events |= MCAL_DMA_EVENT_HALF_TRANSFER;
        clear_mask |= STM32_DMA_IFCR_CHTIF1;
    }

    if ((status & STM32_DMA_ISR_TCIF1) != 0U)
    {
        events |= MCAL_DMA_EVENT_TRANSFER_COMPLETE;
        clear_mask |= STM32_DMA_IFCR_CTCIF1;
    }

    if (clear_mask != 0U)
    {
        STM32_DMA1->IFCR = clear_mask;
    }

    return events;
}
