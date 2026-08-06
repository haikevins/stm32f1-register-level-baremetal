#include "mcal_exti.h"

#include <stddef.h>

#include "cortex_m3.h"
#include "cortex_m3_registers.h"
#include "stm32f103xb.h"
#include "stm32f103xb_irq.h"
#include "stm32f103xb_register_bits.h"

#define MCAL_EXTI_LINE_COUNT       (16U)
#define MCAL_NVIC_PRIORITY_MAX     (15U)
#define MCAL_NVIC_PRIORITY_SHIFT   (4U)
#define MCAL_NVIC_REGISTER_WIDTH   (32U)

static volatile uint32_t g_exti_events;

static uint32_t port_encoding(mcal_gpio_port_t port)
{
    switch (port)
    {
        case MCAL_GPIO_PORT_A:
            return STM32_AFIO_EXTICR_PORT_A;
        case MCAL_GPIO_PORT_B:
            return STM32_AFIO_EXTICR_PORT_B;
        case MCAL_GPIO_PORT_C:
            return STM32_AFIO_EXTICR_PORT_C;
        case MCAL_GPIO_PORT_D:
            return STM32_AFIO_EXTICR_PORT_D;
        case MCAL_GPIO_PORT_E:
            return STM32_AFIO_EXTICR_PORT_E;
        default:
            return UINT32_MAX;
    }
}

static stm32_irq_number_t irq_for_line(uint8_t line)
{
    if (line == 0U)
    {
        return EXTI0_IRQn;
    }

    if (line == 1U)
    {
        return EXTI1_IRQn;
    }

    if (line == 2U)
    {
        return EXTI2_IRQn;
    }

    if (line == 3U)
    {
        return EXTI3_IRQn;
    }

    if (line == 4U)
    {
        return EXTI4_IRQn;
    }

    if (line <= 9U)
    {
        return EXTI9_5_IRQn;
    }

    return EXTI15_10_IRQn;
}

static void nvic_enable_irq(stm32_irq_number_t irq_number, uint8_t priority)
{
    const uint32_t irq = (uint32_t)irq_number;
    const uint32_t register_index = irq / MCAL_NVIC_REGISTER_WIDTH;
    const uint32_t bit_index = irq % MCAL_NVIC_REGISTER_WIDTH;
    const uint32_t mask = UINT32_C(1) << bit_index;

    CORTEX_M3_NVIC->IP[irq] =
        (uint8_t)((uint32_t)priority << MCAL_NVIC_PRIORITY_SHIFT);
    CORTEX_M3_NVIC->ICPR[register_index] = mask;
    CORTEX_M3_NVIC->ISER[register_index] = mask;
}


static void record_pending_lines(uint32_t line_mask)
{
    const uint32_t pending = STM32_EXTI->PR & line_mask;

    if (pending != 0U)
    {
        STM32_EXTI->PR = pending;
        g_exti_events |= pending;
    }
}

bool mcal_exti_configure_line(uint8_t line,
                              mcal_gpio_port_t port,
                              mcal_exti_trigger_t trigger,
                              uint8_t irq_priority)
{
    const uint32_t encoded_port = port_encoding(port);

    if ((line >= MCAL_EXTI_LINE_COUNT) ||
        (encoded_port == UINT32_MAX) ||
        (irq_priority > MCAL_NVIC_PRIORITY_MAX))
    {
        return false;
    }

    if ((trigger != MCAL_EXTI_TRIGGER_RISING) &&
        (trigger != MCAL_EXTI_TRIGGER_FALLING) &&
        (trigger != MCAL_EXTI_TRIGGER_BOTH))
    {
        return false;
    }

    const uint32_t line_mask = UINT32_C(1) << line;
    const uint32_t exticr_index = (uint32_t)line / 4U;
    const uint32_t exticr_shift = ((uint32_t)line % 4U) * 4U;
    const uint32_t exticr_mask =
        STM32_AFIO_EXTICR_PORT_MASK << exticr_shift;

    STM32_RCC->APB2ENR |= STM32_RCC_APB2ENR_AFIOEN;
    (void)STM32_RCC->APB2ENR;

    STM32_EXTI->IMR &= ~line_mask;
    STM32_AFIO->EXTICR[exticr_index] =
        (STM32_AFIO->EXTICR[exticr_index] & ~exticr_mask) |
        (encoded_port << exticr_shift);

    STM32_EXTI->RTSR &= ~line_mask;
    STM32_EXTI->FTSR &= ~line_mask;

    if ((trigger == MCAL_EXTI_TRIGGER_RISING) ||
        (trigger == MCAL_EXTI_TRIGGER_BOTH))
    {
        STM32_EXTI->RTSR |= line_mask;
    }

    if ((trigger == MCAL_EXTI_TRIGGER_FALLING) ||
        (trigger == MCAL_EXTI_TRIGGER_BOTH))
    {
        STM32_EXTI->FTSR |= line_mask;
    }

    STM32_EXTI->PR = line_mask;
    g_exti_events &= ~line_mask;

    nvic_enable_irq(irq_for_line(line), irq_priority);
    STM32_EXTI->IMR |= line_mask;

    return true;
}

void mcal_exti_disable_line(uint8_t line)
{
    if (line >= MCAL_EXTI_LINE_COUNT)
    {
        return;
    }

    const uint32_t line_mask = UINT32_C(1) << line;
    STM32_EXTI->IMR &= ~line_mask;
    STM32_EXTI->RTSR &= ~line_mask;
    STM32_EXTI->FTSR &= ~line_mask;
    STM32_EXTI->PR = line_mask;
    g_exti_events &= ~line_mask;
}

bool mcal_exti_take_event(uint8_t line)
{
    if (line >= MCAL_EXTI_LINE_COUNT)
    {
        return false;
    }

    const uint32_t line_mask = UINT32_C(1) << line;
    const uint32_t saved_primask = cortex_m3_get_primask();

    cortex_m3_disable_irq();

    const bool event_present = (g_exti_events & line_mask) != 0U;
    g_exti_events &= ~line_mask;

    if (saved_primask == 0U)
    {
        cortex_m3_enable_irq();
    }

    return event_present;
}

void EXTI0_IRQHandler(void)
{
    record_pending_lines(UINT32_C(1) << 0U);
}

void EXTI1_IRQHandler(void)
{
    record_pending_lines(UINT32_C(1) << 1U);
}

void EXTI2_IRQHandler(void)
{
    record_pending_lines(UINT32_C(1) << 2U);
}

void EXTI3_IRQHandler(void)
{
    record_pending_lines(UINT32_C(1) << 3U);
}

void EXTI4_IRQHandler(void)
{
    record_pending_lines(UINT32_C(1) << 4U);
}

void EXTI9_5_IRQHandler(void)
{
    record_pending_lines(UINT32_C(0x03E0));
}

void EXTI15_10_IRQHandler(void)
{
    record_pending_lines(UINT32_C(0xFC00));
}
