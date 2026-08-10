#include "mcal_gpio.h"

#include <stddef.h>

#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

static stm32_gpio_registers_t *gpio_registers(mcal_gpio_port_t port)
{
    switch (port)
    {
        case MCAL_GPIO_PORT_A:
            return STM32_GPIOA;
        case MCAL_GPIO_PORT_B:
            return STM32_GPIOB;
        case MCAL_GPIO_PORT_C:
            return STM32_GPIOC;
        case MCAL_GPIO_PORT_D:
            return STM32_GPIOD;
        case MCAL_GPIO_PORT_E:
            return STM32_GPIOE;
        default:
            return NULL;
    }
}

static uint32_t gpio_clock_mask(mcal_gpio_port_t port)
{
    switch (port)
    {
        case MCAL_GPIO_PORT_A:
            return STM32_RCC_APB2ENR_IOPAEN;
        case MCAL_GPIO_PORT_B:
            return STM32_RCC_APB2ENR_IOPBEN;
        case MCAL_GPIO_PORT_C:
            return STM32_RCC_APB2ENR_IOPCEN;
        case MCAL_GPIO_PORT_D:
            return STM32_RCC_APB2ENR_IOPDEN;
        case MCAL_GPIO_PORT_E:
            return STM32_RCC_APB2ENR_IOPEEN;
        default:
            return 0U;
    }
}

bool mcal_gpio_configure(mcal_gpio_port_t port,
                         uint8_t pin,
                         mcal_gpio_mode_t mode,
                         mcal_gpio_level_t initial_or_pull_level)
{
    stm32_gpio_registers_t *gpio = gpio_registers(port);
    const uint32_t clock_mask = gpio_clock_mask(port);

    if ((gpio == NULL) || (clock_mask == 0U) || (pin > 15U))
    {
        return false;
    }

    STM32_RCC->APB2ENR |= clock_mask;
    (void)STM32_RCC->APB2ENR;

    volatile uint32_t *configuration_register;
    uint32_t local_pin;

    if (pin < 8U)
    {
        configuration_register = &gpio->CRL;
        local_pin = pin;
    }
    else
    {
        configuration_register = &gpio->CRH;
        local_pin = (uint32_t)pin - 8U;
    }

    const uint32_t shift = local_pin * 4U;
    const uint32_t mask = UINT32_C(0xF) << shift;
    const uint32_t value = ((uint32_t)mode & UINT32_C(0xF)) << shift;

    /*
     * Load the requested output/pull level before changing the pin mode.
     * This prevents a brief unwanted output level when a pin such as an
     * active-low chip-select transitions from reset input state to output.
     */
    if ((mode == MCAL_GPIO_MODE_INPUT_PULL) ||
        (((uint32_t)mode & UINT32_C(0x3)) != 0U))
    {
        mcal_gpio_write(port, pin, initial_or_pull_level);
    }

    *configuration_register = (*configuration_register & ~mask) | value;

    return true;
}

void mcal_gpio_write(mcal_gpio_port_t port,
                     uint8_t pin,
                     mcal_gpio_level_t level)
{
    stm32_gpio_registers_t *gpio = gpio_registers(port);

    if ((gpio == NULL) || (pin > 15U))
    {
        return;
    }

    if (level == MCAL_GPIO_LEVEL_HIGH)
    {
        gpio->BSRR = UINT32_C(1) << pin;
    }
    else
    {
        gpio->BSRR = UINT32_C(1) << ((uint32_t)pin + 16U);
    }
}

mcal_gpio_level_t mcal_gpio_read(mcal_gpio_port_t port, uint8_t pin)
{
    stm32_gpio_registers_t *gpio = gpio_registers(port);

    if ((gpio == NULL) || (pin > 15U))
    {
        return MCAL_GPIO_LEVEL_LOW;
    }

    return ((gpio->IDR & (UINT32_C(1) << pin)) != 0U)
               ? MCAL_GPIO_LEVEL_HIGH
               : MCAL_GPIO_LEVEL_LOW;
}

void mcal_gpio_toggle(mcal_gpio_port_t port, uint8_t pin)
{
    stm32_gpio_registers_t *gpio = gpio_registers(port);

    if ((gpio == NULL) || (pin > 15U))
    {
        return;
    }

    const bool currently_high = (gpio->ODR & (UINT32_C(1) << pin)) != 0U;
    mcal_gpio_write(port,
                    pin,
                    currently_high ? MCAL_GPIO_LEVEL_LOW : MCAL_GPIO_LEVEL_HIGH);
}
