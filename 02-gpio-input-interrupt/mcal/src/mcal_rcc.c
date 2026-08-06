#include "mcal_rcc.h"

#include <stdbool.h>

#include "mcal_config.h"
#include "stm32f103xb.h"
#include "stm32f103xb_register_bits.h"

#define STM32_HSI_FREQUENCY_HZ (8000000UL)
#define STM32_MAX_SYSCLK_HZ     (72000000UL)
#define STM32_MAX_APB1_HZ       (36000000UL)

static uint32_t g_system_clock_hz = STM32_HSI_FREQUENCY_HZ;

static bool wait_for_mask(volatile const uint32_t *reg,
                          uint32_t mask,
                          uint32_t expected)
{
    uint32_t timeout = MCAL_RCC_READY_TIMEOUT_CYCLES;

    while (timeout > 0U)
    {
        if ((*reg & mask) == expected)
        {
            return true;
        }
        timeout--;
    }

    return false;
}

static uint32_t flash_latency_for_clock(uint32_t clock_hz)
{
    if (clock_hz <= 24000000UL)
    {
        return STM32_FLASH_ACR_LATENCY_0;
    }

    if (clock_hz <= 48000000UL)
    {
        return STM32_FLASH_ACR_LATENCY_1;
    }

    return STM32_FLASH_ACR_LATENCY_2;
}

mcal_rcc_clock_status_t mcal_rcc_configure_hse_pll(uint32_t hse_hz,
                                                    uint32_t target_hz)
{
    if ((hse_hz == 0U) || (target_hz > STM32_MAX_SYSCLK_HZ) ||
        ((target_hz % hse_hz) != 0U))
    {
        return MCAL_RCC_CLOCK_INVALID_CONFIGURATION;
    }

    const uint32_t multiplier = target_hz / hse_hz;

    if ((multiplier < 2U) || (multiplier > 16U))
    {
        return MCAL_RCC_CLOCK_INVALID_CONFIGURATION;
    }

    STM32_RCC->CR |= STM32_RCC_CR_HSION;

    if (!wait_for_mask(&STM32_RCC->CR,
                       STM32_RCC_CR_HSIRDY,
                       STM32_RCC_CR_HSIRDY))
    {
        return MCAL_RCC_CLOCK_SWITCH_TIMEOUT;
    }

    STM32_RCC->CFGR = (STM32_RCC->CFGR & ~STM32_RCC_CFGR_SW_MASK) |
                      STM32_RCC_CFGR_SW_HSI;

    if (!wait_for_mask(&STM32_RCC->CFGR,
                       STM32_RCC_CFGR_SWS_MASK,
                       STM32_RCC_CFGR_SWS_HSI))
    {
        return MCAL_RCC_CLOCK_SWITCH_TIMEOUT;
    }

    STM32_RCC->CR &= ~STM32_RCC_CR_PLLON;

    if (!wait_for_mask(&STM32_RCC->CR, STM32_RCC_CR_PLLRDY, 0U))
    {
        return MCAL_RCC_CLOCK_PLL_TIMEOUT;
    }

    STM32_RCC->CR |= STM32_RCC_CR_HSEON;

    if (!wait_for_mask(&STM32_RCC->CR,
                       STM32_RCC_CR_HSERDY,
                       STM32_RCC_CR_HSERDY))
    {
        mcal_rcc_use_hsi();
        return MCAL_RCC_CLOCK_HSE_TIMEOUT;
    }

    STM32_FLASH->ACR = STM32_FLASH_ACR_PRFTBE |
                       flash_latency_for_clock(target_hz);

    uint32_t cfgr = STM32_RCC->CFGR;
    cfgr &= ~(STM32_RCC_CFGR_HPRE_MASK |
              STM32_RCC_CFGR_PPRE1_MASK |
              STM32_RCC_CFGR_PPRE2_MASK |
              STM32_RCC_CFGR_PLLMUL_MASK |
              STM32_RCC_CFGR_PLLXTPRE |
              STM32_RCC_CFGR_SW_MASK);

    if (target_hz > STM32_MAX_APB1_HZ)
    {
        cfgr |= STM32_RCC_CFGR_PPRE1_DIV2;
    }
    else
    {
        cfgr |= STM32_RCC_CFGR_PPRE1_DIV1;
    }

    cfgr |= STM32_RCC_CFGR_PLLSRC_HSE |
            STM32_RCC_CFGR_PLLMUL_ENCODE(multiplier);
    STM32_RCC->CFGR = cfgr;

    STM32_RCC->CR |= STM32_RCC_CR_PLLON;

    if (!wait_for_mask(&STM32_RCC->CR,
                       STM32_RCC_CR_PLLRDY,
                       STM32_RCC_CR_PLLRDY))
    {
        mcal_rcc_use_hsi();
        return MCAL_RCC_CLOCK_PLL_TIMEOUT;
    }

    STM32_RCC->CFGR = (STM32_RCC->CFGR & ~STM32_RCC_CFGR_SW_MASK) |
                      STM32_RCC_CFGR_SW_PLL;

    if (!wait_for_mask(&STM32_RCC->CFGR,
                       STM32_RCC_CFGR_SWS_MASK,
                       STM32_RCC_CFGR_SWS_PLL))
    {
        mcal_rcc_use_hsi();
        return MCAL_RCC_CLOCK_SWITCH_TIMEOUT;
    }

    g_system_clock_hz = target_hz;
    return MCAL_RCC_CLOCK_OK;
}

void mcal_rcc_use_hsi(void)
{
    STM32_RCC->CR |= STM32_RCC_CR_HSION;
    (void)wait_for_mask(&STM32_RCC->CR,
                        STM32_RCC_CR_HSIRDY,
                        STM32_RCC_CR_HSIRDY);

    STM32_RCC->CFGR = (STM32_RCC->CFGR & ~STM32_RCC_CFGR_SW_MASK) |
                      STM32_RCC_CFGR_SW_HSI;
    (void)wait_for_mask(&STM32_RCC->CFGR,
                        STM32_RCC_CFGR_SWS_MASK,
                        STM32_RCC_CFGR_SWS_HSI);

    STM32_RCC->CR &= ~(STM32_RCC_CR_PLLON | STM32_RCC_CR_HSEON);
    (void)wait_for_mask(&STM32_RCC->CR, STM32_RCC_CR_PLLRDY, 0U);

    STM32_RCC->CFGR &= ~(STM32_RCC_CFGR_HPRE_MASK |
                         STM32_RCC_CFGR_PPRE1_MASK |
                         STM32_RCC_CFGR_PPRE2_MASK |
                         STM32_RCC_CFGR_PLLMUL_MASK |
                         STM32_RCC_CFGR_PLLXTPRE |
                         STM32_RCC_CFGR_PLLSRC_HSE);

    STM32_FLASH->ACR = STM32_FLASH_ACR_PRFTBE |
                       STM32_FLASH_ACR_LATENCY_0;

    g_system_clock_hz = STM32_HSI_FREQUENCY_HZ;
}

uint32_t mcal_rcc_get_system_clock_hz(void)
{
    return g_system_clock_hz;
}
