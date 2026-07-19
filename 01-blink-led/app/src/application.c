#include "application.h"

#include "application_config.h"
#include "indication_service.h"
#include "time_service.h"

static uint32_t g_last_blink_ms;

void application_init(void)
{
    g_last_blink_ms = time_service_now_ms();
    indication_service_set(INDICATION_STATUS, false);
}

void application_process(void)
{
    if (time_service_periodic_due(&g_last_blink_ms,
                                  APPLICATION_BLINK_PERIOD_MS))
    {
        indication_service_toggle(INDICATION_STATUS);
    }
}
