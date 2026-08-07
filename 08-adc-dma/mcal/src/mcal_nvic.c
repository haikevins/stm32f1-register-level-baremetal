#include "mcal_nvic.h"

#include "cortex_m3_registers.h"
#include "stm32f103xb_irq.h"

#define NVIC_PRIORITY_MAX      (15U)
#define NVIC_PRIORITY_SHIFT    (4U)
#define NVIC_REGISTER_WIDTH    (32U)

static bool map_irq(mcal_nvic_irq_t irq,
                    stm32_irq_number_t *irq_number)
{
    if (irq_number == 0)
    {
        return false;
    }

    switch (irq)
    {
        case MCAL_NVIC_IRQ_DMA1_CHANNEL1:
            *irq_number = DMA1_Channel1_IRQn;
            return true;
        default:
            return false;
    }
}

bool mcal_nvic_enable_irq(mcal_nvic_irq_t irq,
                          uint8_t priority)
{
    stm32_irq_number_t mapped_irq;
    uint32_t irq_index;
    uint32_t register_index;
    uint32_t bit_index;
    uint32_t mask;

    if ((priority > NVIC_PRIORITY_MAX) ||
        !map_irq(irq, &mapped_irq))
    {
        return false;
    }

    irq_index = (uint32_t)mapped_irq;
    register_index = irq_index / NVIC_REGISTER_WIDTH;
    bit_index = irq_index % NVIC_REGISTER_WIDTH;
    mask = UINT32_C(1) << bit_index;

    CORTEX_M3_NVIC->IP[irq_index] =
        (uint8_t)((uint32_t)priority << NVIC_PRIORITY_SHIFT);
    CORTEX_M3_NVIC->ICPR[register_index] = mask;
    CORTEX_M3_NVIC->ISER[register_index] = mask;

    return true;
}

void mcal_nvic_disable_irq(mcal_nvic_irq_t irq)
{
    stm32_irq_number_t mapped_irq;
    uint32_t irq_index;
    uint32_t register_index;
    uint32_t bit_index;
    uint32_t mask;

    if (!map_irq(irq, &mapped_irq))
    {
        return;
    }

    irq_index = (uint32_t)mapped_irq;
    register_index = irq_index / NVIC_REGISTER_WIDTH;
    bit_index = irq_index % NVIC_REGISTER_WIDTH;
    mask = UINT32_C(1) << bit_index;

    CORTEX_M3_NVIC->ICER[register_index] = mask;
    CORTEX_M3_NVIC->ICPR[register_index] = mask;
}
