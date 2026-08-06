#include "system.h"

#include "cortex_m3.h"

void system_idle(void)
{
    cortex_m3_nop();
}

void system_panic(void)
{
    cortex_m3_disable_irq();

    for (;;)
    {
        cortex_m3_nop();
    }
}
