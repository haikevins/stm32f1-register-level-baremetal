#include "mcal_nvic.h"
#include "cortex_m3_registers.h"
#include "stm32f103xb_irq.h"
#define NVIC_PRIORITY_MAX (15U)
#define NVIC_PRIORITY_SHIFT (4U)
static bool lookup(mcal_nvic_irq_t irq, uint32_t *out)
{
    if ((out == 0) || (irq != MCAL_NVIC_IRQ_CAN1_RX0)) { return false; }
    *out = (uint32_t)USB_LP_CAN1_RX0_IRQn;
    return true;
}
bool mcal_nvic_enable_irq(mcal_nvic_irq_t irq, uint8_t priority)
{
    uint32_t num;
    uint32_t mask;
    if ((priority > NVIC_PRIORITY_MAX) || !lookup(irq, &num)) { return false; }
    mask = UINT32_C(1) << (num % 32U);
    CORTEX_M3_NVIC->IP[num] = (uint8_t)((uint32_t)priority << NVIC_PRIORITY_SHIFT);
    CORTEX_M3_NVIC->ICPR[num / 32U] = mask;
    CORTEX_M3_NVIC->ISER[num / 32U] = mask;
    return true;
}
void mcal_nvic_disable_irq(mcal_nvic_irq_t irq)
{
    uint32_t num;
    if (!lookup(irq, &num)) { return; }
    CORTEX_M3_NVIC->ICER[num / 32U] = UINT32_C(1) << (num % 32U);
}
