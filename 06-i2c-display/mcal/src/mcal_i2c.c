#include "mcal_i2c.h"

#include <stddef.h>

#include "mcal_config.h"
#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

#define I2C_CLOCK_MIN_HZ       (1000000UL)
#define I2C_CLOCK_MAX_HZ       (36000000UL)
#define I2C_STANDARD_MAX_HZ    (100000UL)
#define I2C_FAST_MAX_HZ        (400000UL)
#define I2C_CCR_MAX            (0x0FFFUL)
#define I2C_CR2_FREQ_MAX_MHZ   (36UL)

static stm32_i2c_registers_t *i2c_registers(mcal_i2c_instance_t instance)
{
    switch (instance)
    {
        case MCAL_I2C_INSTANCE_1:
            return STM32_I2C1;
        default:
            return NULL;
    }
}

static uint32_t i2c_clock_enable_mask(mcal_i2c_instance_t instance)
{
    switch (instance)
    {
        case MCAL_I2C_INSTANCE_1:
            return STM32_RCC_APB1ENR_I2C1EN;
        default:
            return 0U;
    }
}

static uint32_t i2c_reset_mask(mcal_i2c_instance_t instance)
{
    switch (instance)
    {
        case MCAL_I2C_INSTANCE_1:
            return STM32_RCC_APB1RSTR_I2C1RST;
        default:
            return 0U;
    }
}

static uint32_t divide_round_up(uint32_t numerator, uint32_t denominator)
{
    return (numerator + denominator - 1U) / denominator;
}

static bool i2c_error_pending(const stm32_i2c_registers_t *i2c)
{
    return (i2c->SR1 & STM32_I2C_SR1_ERROR_MASK) != 0U;
}

static void i2c_clear_errors(stm32_i2c_registers_t *i2c)
{
    i2c->SR1 &= ~STM32_I2C_SR1_ERROR_MASK;
}

static bool wait_sr1_set(stm32_i2c_registers_t *i2c, uint32_t mask)
{
    uint32_t timeout = MCAL_I2C_POLL_TIMEOUT_CYCLES;

    while (timeout > 0U)
    {
        if ((i2c->SR1 & mask) == mask)
        {
            return true;
        }

        if (i2c_error_pending(i2c))
        {
            return false;
        }

        timeout--;
    }

    return false;
}

static bool wait_bus_idle(stm32_i2c_registers_t *i2c)
{
    uint32_t timeout = MCAL_I2C_POLL_TIMEOUT_CYCLES;

    while (timeout > 0U)
    {
        if ((i2c->SR2 & STM32_I2C_SR2_BUSY) == 0U)
        {
            return true;
        }

        timeout--;
    }

    return false;
}

static void abort_transfer(stm32_i2c_registers_t *i2c)
{
    i2c->CR1 |= STM32_I2C_CR1_STOP;
    i2c_clear_errors(i2c);
}

bool mcal_i2c_init(mcal_i2c_instance_t instance,
                   uint32_t peripheral_clock_hz,
                   uint32_t bus_clock_hz)
{
    stm32_i2c_registers_t *i2c = i2c_registers(instance);
    const uint32_t clock_mask = i2c_clock_enable_mask(instance);
    const uint32_t reset_mask = i2c_reset_mask(instance);
    uint32_t peripheral_clock_mhz;
    uint32_t ccr;
    uint32_t trise;

    if ((i2c == NULL) ||
        (clock_mask == 0U) ||
        (reset_mask == 0U) ||
        (peripheral_clock_hz < I2C_CLOCK_MIN_HZ) ||
        (peripheral_clock_hz > I2C_CLOCK_MAX_HZ) ||
        ((peripheral_clock_hz % 1000000UL) != 0U) ||
        (bus_clock_hz == 0U) ||
        (bus_clock_hz > I2C_FAST_MAX_HZ))
    {
        return false;
    }

    peripheral_clock_mhz = peripheral_clock_hz / 1000000UL;

    if ((peripheral_clock_mhz < 2U) ||
        (peripheral_clock_mhz > I2C_CR2_FREQ_MAX_MHZ))
    {
        return false;
    }

    STM32_RCC->APB1ENR |= clock_mask;
    (void)STM32_RCC->APB1ENR;

    STM32_RCC->APB1RSTR |= reset_mask;
    STM32_RCC->APB1RSTR &= ~reset_mask;

    i2c->CR1 = 0U;
    i2c->CR2 = peripheral_clock_mhz & STM32_I2C_CR2_FREQ_MASK;
    i2c->OAR1 = STM32_I2C_OAR1_BIT14;
    i2c->OAR2 = 0U;

    if (bus_clock_hz <= I2C_STANDARD_MAX_HZ)
    {
        ccr = divide_round_up(peripheral_clock_hz,
                              2U * bus_clock_hz);

        if (ccr < 4U)
        {
            ccr = 4U;
        }

        trise = peripheral_clock_mhz + 1U;
        i2c->CCR = ccr & STM32_I2C_CCR_CCR_MASK;
    }
    else
    {
        /*
         * Fast mode, duty cycle 2:
         * Thigh = CCR * TPCLK1
         * Tlow  = 2 * CCR * TPCLK1
         */
        ccr = divide_round_up(peripheral_clock_hz,
                              3U * bus_clock_hz);

        if (ccr == 0U)
        {
            ccr = 1U;
        }

        trise =
            ((peripheral_clock_mhz * 300U) / 1000U) + 1U;

        i2c->CCR =
            STM32_I2C_CCR_FS |
            (ccr & STM32_I2C_CCR_CCR_MASK);
    }

    if ((ccr > I2C_CCR_MAX) ||
        (trise == 0U) ||
        (trise > STM32_I2C_TRISE_MASK))
    {
        return false;
    }

    i2c->TRISE = trise & STM32_I2C_TRISE_MASK;
    i2c_clear_errors(i2c);
    i2c->CR1 = STM32_I2C_CR1_PE;

    return wait_bus_idle(i2c);
}

bool mcal_i2c_master_write_prefixed(mcal_i2c_instance_t instance,
                                    uint8_t address_7bit,
                                    uint8_t prefix,
                                    const uint8_t *data,
                                    size_t length)
{
    stm32_i2c_registers_t *i2c = i2c_registers(instance);
    size_t index;
    volatile uint32_t clear_sequence;

    if ((i2c == NULL) ||
        (address_7bit > 0x7FU) ||
        ((data == NULL) && (length != 0U)))
    {
        return false;
    }

    i2c_clear_errors(i2c);

    if (!wait_bus_idle(i2c))
    {
        return false;
    }

    i2c->CR1 |= STM32_I2C_CR1_START;

    if (!wait_sr1_set(i2c, STM32_I2C_SR1_SB))
    {
        abort_transfer(i2c);
        return false;
    }

    i2c->DR = ((uint32_t)address_7bit << 1U);

    if (!wait_sr1_set(i2c, STM32_I2C_SR1_ADDR))
    {
        abort_transfer(i2c);
        return false;
    }

    /*
     * Clear ADDR by the mandatory SR1 read followed by SR2 read.
     */
    clear_sequence = i2c->SR1;
    clear_sequence = i2c->SR2;
    (void)clear_sequence;

    if (!wait_sr1_set(i2c, STM32_I2C_SR1_TXE))
    {
        abort_transfer(i2c);
        return false;
    }

    i2c->DR = prefix;

    for (index = 0U; index < length; index++)
    {
        if (!wait_sr1_set(i2c, STM32_I2C_SR1_TXE))
        {
            abort_transfer(i2c);
            return false;
        }

        i2c->DR = data[index];
    }

    if (!wait_sr1_set(i2c, STM32_I2C_SR1_BTF))
    {
        abort_transfer(i2c);
        return false;
    }

    i2c->CR1 |= STM32_I2C_CR1_STOP;

    return true;
}
