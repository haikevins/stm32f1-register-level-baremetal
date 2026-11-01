#include "cortex_m3.h"
#include "cortex_m3_registers.h"

void cortex_m3_system_reset(void)
{
    cortex_m3_disable_irq();
    cortex_m3_data_sync_barrier();

    CORTEX_M3_SCB->AIRCR = CORTEX_M3_SCB_AIRCR_VECTKEY |
                           CORTEX_M3_SCB_AIRCR_SYSRESETREQ;

    cortex_m3_data_sync_barrier();

    for (;;)
    {
        cortex_m3_wait_for_interrupt();
    }
}
