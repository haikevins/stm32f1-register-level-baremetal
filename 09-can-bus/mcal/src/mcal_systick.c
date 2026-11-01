#include "mcal_systick.h"

#include "cortex_m3_registers.h"

#define SYSTICK_RELOAD_MAX (UINT32_C(0x00FFFFFF))

static volatile uint32_t g_systick_ticks;

bool mcal_systick_init(uint32_t core_clock_hz, uint32_t tick_hz)
{
    if ((core_clock_hz == 0U) || (tick_hz == 0U) ||
        ((core_clock_hz % tick_hz) != 0U))
    {
        return false;
    }

    const uint32_t reload_value = (core_clock_hz / tick_hz) - 1U;

    if (reload_value > SYSTICK_RELOAD_MAX)
    {
        return false;
    }

    g_systick_ticks = 0U;
    CORTEX_M3_SYSTICK->CTRL = 0U;
    CORTEX_M3_SYSTICK->LOAD = reload_value;
    CORTEX_M3_SYSTICK->VAL = 0U;
    CORTEX_M3_SYSTICK->CTRL = CORTEX_M3_SYSTICK_CTRL_CLKSOURCE |
                              CORTEX_M3_SYSTICK_CTRL_TICKINT |
                              CORTEX_M3_SYSTICK_CTRL_ENABLE;

    return true;
}

void mcal_systick_deinit(void)
{
    CORTEX_M3_SYSTICK->CTRL = 0U;
    CORTEX_M3_SYSTICK->LOAD = 0U;
    CORTEX_M3_SYSTICK->VAL = 0U;
    g_systick_ticks = 0U;
}

uint32_t mcal_systick_get_ticks(void)
{
    return g_systick_ticks;
}

void SysTick_Handler(void)
{
    g_systick_ticks++;
}
