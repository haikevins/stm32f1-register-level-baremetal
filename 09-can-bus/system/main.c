#include "system.h"

#include "application.h"
#include "cortex_m3.h"

int main(void)
{
    cortex_m3_disable_irq();

    if (!system_init())
    {
        system_panic();
    }

    cortex_m3_enable_irq();

    for (;;)
    {
        application_process();
        system_idle();
    }
}
