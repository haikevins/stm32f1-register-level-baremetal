#include "mcal_delay.h"

#include "cortex_m3.h"

void mcal_delay_busy_ms(uint32_t core_clock_hz, uint32_t delay_ms)
{
    const uint32_t loops_per_ms = core_clock_hz / 1000U;
    uint32_t millisecond;

    if ((loops_per_ms == 0U) || (delay_ms == 0U))
    {
        return;
    }

    for (millisecond = 0U; millisecond < delay_ms; millisecond++)
    {
        uint32_t loop;

        for (loop = 0U; loop < loops_per_ms; loop++)
        {
            cortex_m3_nop();
        }
    }
}
