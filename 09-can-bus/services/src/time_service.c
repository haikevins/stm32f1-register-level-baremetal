#include "time_service.h"

#include <stddef.h>

#include "board_timebase.h"

void time_service_init(void)
{
    /* Board timebase is initialized by the composition root before services. */
}

uint32_t time_service_now_ms(void)
{
    return board_timebase_now_ms();
}

uint32_t time_service_elapsed_ms(uint32_t start_time_ms)
{
    return time_service_now_ms() - start_time_ms;
}

bool time_service_periodic_due(uint32_t *last_run_ms, uint32_t period_ms)
{
    if ((last_run_ms == NULL) || (period_ms == 0U))
    {
        return false;
    }

    const uint32_t now_ms = time_service_now_ms();

    if ((uint32_t)(now_ms - *last_run_ms) < period_ms)
    {
        return false;
    }

    *last_run_ms = now_ms;
    return true;
}
