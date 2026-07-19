#include "system.h"

#include "cortex_m3.h"

void system_idle(void)
{
    cortex_m3_wait_for_interrupt();
}

void system_panic(void)
{
    cortex_m3_disable_irq();

    for (;;)
    {
        __asm volatile("nop");
    }
}
