#include "system.h"

#include "application.h"
#include "board.h"
#include "event_service.h"
#include "indication_service.h"
#include "time_service.h"

bool system_init(void)
{
    if (!board_init())
    {
        return false;
    }

    time_service_init();
    indication_service_init();

    if (!event_service_init())
    {
        return false;
    }

    application_init();
    return true;
}
