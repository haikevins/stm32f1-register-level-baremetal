#include "mcal_irq.h"

#include "cortex_m3.h"

uint32_t mcal_irq_save_and_disable(void)
{
    const uint32_t saved_primask = cortex_m3_get_primask();
    cortex_m3_disable_irq();
    return saved_primask;
}

void mcal_irq_restore(uint32_t saved_primask)
{
    if (saved_primask == 0U)
    {
        cortex_m3_enable_irq();
    }
}
