#ifndef MCAL_RCC_H
#define MCAL_RCC_H

#include <stdint.h>

typedef enum
{
    MCAL_RCC_CLOCK_OK = 0,
    MCAL_RCC_CLOCK_INVALID_CONFIGURATION,
    MCAL_RCC_CLOCK_HSE_TIMEOUT,
    MCAL_RCC_CLOCK_PLL_TIMEOUT,
    MCAL_RCC_CLOCK_SWITCH_TIMEOUT
} mcal_rcc_clock_status_t;

mcal_rcc_clock_status_t mcal_rcc_configure_hse_pll(uint32_t hse_hz,
                                                    uint32_t target_hz);
void mcal_rcc_use_hsi(void);
uint32_t mcal_rcc_get_system_clock_hz(void);

#endif
