#include "mcal_usart.h"

#include <stddef.h>

#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

static uint32_t g_error_flags[MCAL_USART_INSTANCE_COUNT];

static stm32_usart_registers_t *usart_registers(
    mcal_usart_instance_t instance)
{
    switch (instance)
    {
        case MCAL_USART_INSTANCE_1:
            return STM32_USART1;
        default:
            return NULL;
    }
}

static uint32_t usart_clock_mask(mcal_usart_instance_t instance)
{
    switch (instance)
    {
        case MCAL_USART_INSTANCE_1:
            return STM32_RCC_APB2ENR_USART1EN;
        default:
            return 0U;
    }
}

static uint32_t portable_error_flags(uint32_t status)
{
    uint32_t flags = 0U;

    if ((status & STM32_USART_SR_PE) != 0U)
    {
        flags |= MCAL_USART_ERROR_PARITY;
    }

    if ((status & STM32_USART_SR_FE) != 0U)
    {
        flags |= MCAL_USART_ERROR_FRAMING;
    }

    if ((status & STM32_USART_SR_NE) != 0U)
    {
        flags |= MCAL_USART_ERROR_NOISE;
    }

    if ((status & STM32_USART_SR_ORE) != 0U)
    {
        flags |= MCAL_USART_ERROR_OVERRUN;
    }

    return flags;
}

bool mcal_usart_init(mcal_usart_instance_t instance,
                     uint32_t peripheral_clock_hz,
                     uint32_t baud_rate)
{
    stm32_usart_registers_t *usart = usart_registers(instance);
    const uint32_t clock_mask = usart_clock_mask(instance);

    if ((usart == NULL) ||
        (clock_mask == 0U) ||
        (peripheral_clock_hz == 0U) ||
        (baud_rate == 0U))
    {
        return false;
    }

    /*
     * For oversampling by 16, BRR encodes 16 * USARTDIV, which is
     * approximately peripheral_clock_hz / baud_rate. Add half the divisor
     * before division to round to the nearest representable baud rate.
     */
    const uint32_t baud_divider =
        (peripheral_clock_hz + (baud_rate / 2U)) / baud_rate;

    if ((baud_divider < 16U) || (baud_divider > UINT32_C(0xFFFF)))
    {
        return false;
    }

    STM32_RCC->APB2ENR |= clock_mask;
    (void)STM32_RCC->APB2ENR;

    usart->CR1 = 0U;
    usart->CR2 = 0U;
    usart->CR3 = 0U;
    usart->BRR = baud_divider;

    /*
     * Default reset values select 8 data bits, no parity and one stop bit.
     * Enable transmitter, receiver and USART after all configuration writes.
     */
    usart->CR1 = STM32_USART_CR1_TE |
                 STM32_USART_CR1_RE |
                 STM32_USART_CR1_UE;

    g_error_flags[instance] = 0U;
    return true;
}

bool mcal_usart_try_read_byte(mcal_usart_instance_t instance, uint8_t *byte)
{
    stm32_usart_registers_t *usart = usart_registers(instance);

    if ((usart == NULL) || (byte == NULL))
    {
        return false;
    }

    const uint32_t status = usart->SR;
    const uint32_t hardware_errors = status & STM32_USART_SR_ERROR_MASK;

    if ((hardware_errors != 0U) ||
        ((status & STM32_USART_SR_RXNE) != 0U))
    {
        /*
         * Reading SR followed by DR clears RXNE and PE/FE/NE/ORE on STM32F1.
         */
        const uint32_t received_data = usart->DR;

        if (hardware_errors != 0U)
        {
            g_error_flags[instance] |= portable_error_flags(hardware_errors);
            return false;
        }

        *byte = (uint8_t)(received_data & UINT32_C(0xFF));
        return true;
    }

    return false;
}

bool mcal_usart_try_write_byte(mcal_usart_instance_t instance, uint8_t byte)
{
    stm32_usart_registers_t *usart = usart_registers(instance);

    if (usart == NULL)
    {
        return false;
    }

    if ((usart->SR & STM32_USART_SR_TXE) == 0U)
    {
        return false;
    }

    usart->DR = byte;
    return true;
}

uint32_t mcal_usart_take_error_flags(mcal_usart_instance_t instance)
{
    if ((uint32_t)instance >= (uint32_t)MCAL_USART_INSTANCE_COUNT)
    {
        return 0U;
    }

    const uint32_t flags = g_error_flags[instance];
    g_error_flags[instance] = 0U;
    return flags;
}
