#include "mcal_spi.h"

#include <stddef.h>

#include "mcal_config.h"
#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

static stm32_spi_registers_t *spi_registers(mcal_spi_instance_t instance)
{
    switch (instance)
    {
        case MCAL_SPI_INSTANCE_1:
            return STM32_SPI1;
        default:
            return NULL;
    }
}

static uint32_t spi_clock_enable_mask(mcal_spi_instance_t instance)
{
    switch (instance)
    {
        case MCAL_SPI_INSTANCE_1:
            return STM32_RCC_APB2ENR_SPI1EN;
        default:
            return 0U;
    }
}

static bool select_baud_rate(uint32_t peripheral_clock_hz,
                             uint32_t maximum_spi_clock_hz,
                             uint32_t *br_bits)
{
    static const uint16_t divisors[8] =
    {
        2U, 4U, 8U, 16U, 32U, 64U, 128U, 256U
    };
    uint32_t index;

    if ((peripheral_clock_hz == 0U) ||
        (maximum_spi_clock_hz == 0U) ||
        (br_bits == NULL))
    {
        return false;
    }

    for (index = 0U; index < 8U; index++)
    {
        const uint32_t divisor = divisors[index];
        const uint32_t rounded_up_clock =
            (peripheral_clock_hz + divisor - 1U) / divisor;

        if (rounded_up_clock <= maximum_spi_clock_hz)
        {
            *br_bits = index << STM32_SPI_CR1_BR_SHIFT;
            return true;
        }
    }

    return false;
}

static bool wait_sr(stm32_spi_registers_t *spi,
                    uint32_t mask,
                    bool set)
{
    uint32_t timeout = MCAL_SPI_POLL_TIMEOUT_CYCLES;

    while (timeout > 0U)
    {
        const bool is_set = (spi->SR & mask) != 0U;

        if (is_set == set)
        {
            return true;
        }

        timeout--;
    }

    return false;
}

bool mcal_spi_init(mcal_spi_instance_t instance,
                   uint32_t peripheral_clock_hz,
                   uint32_t maximum_spi_clock_hz,
                   mcal_spi_mode_t mode)
{
    stm32_spi_registers_t *spi = spi_registers(instance);
    const uint32_t clock_mask = spi_clock_enable_mask(instance);
    uint32_t baud_rate_bits;
    uint32_t cr1;

    if ((spi == NULL) ||
        (clock_mask == 0U) ||
        ((mode != MCAL_SPI_MODE_0) &&
         (mode != MCAL_SPI_MODE_3)) ||
        !select_baud_rate(peripheral_clock_hz,
                          maximum_spi_clock_hz,
                          &baud_rate_bits))
    {
        return false;
    }

    STM32_RCC->APB2ENR |= clock_mask;
    (void)STM32_RCC->APB2ENR;

    spi->CR1 = 0U;
    spi->CR2 = 0U;

    cr1 = STM32_SPI_CR1_MSTR |
          STM32_SPI_CR1_SSM |
          STM32_SPI_CR1_SSI |
          baud_rate_bits;

    if (mode == MCAL_SPI_MODE_3)
    {
        cr1 |= STM32_SPI_CR1_CPOL |
               STM32_SPI_CR1_CPHA;
    }

    spi->CR1 = cr1;
    spi->CR1 |= STM32_SPI_CR1_SPE;

    return true;
}

bool mcal_spi_transfer(mcal_spi_instance_t instance,
                       const uint8_t *transmit_data,
                       uint8_t *receive_data,
                       size_t length)
{
    stm32_spi_registers_t *spi = spi_registers(instance);
    size_t index;

    if ((spi == NULL) ||
        ((transmit_data == NULL) &&
         (receive_data == NULL) &&
         (length != 0U)))
    {
        return false;
    }

    for (index = 0U; index < length; index++)
    {
        const uint8_t transmit_byte =
            (transmit_data != NULL)
                ? transmit_data[index]
                : 0xFFU;
        uint8_t received_byte;

        if (!wait_sr(spi, STM32_SPI_SR_TXE, true))
        {
            return false;
        }

        spi->DR = transmit_byte;

        if (!wait_sr(spi, STM32_SPI_SR_RXNE, true))
        {
            return false;
        }

        received_byte = (uint8_t)spi->DR;

        if (receive_data != NULL)
        {
            receive_data[index] = received_byte;
        }
    }

    if (!wait_sr(spi, STM32_SPI_SR_BSY, false))
    {
        return false;
    }

    return true;
}
