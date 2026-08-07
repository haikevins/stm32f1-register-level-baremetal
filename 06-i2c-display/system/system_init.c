#include "system.h"

#include "application.h"
#include "board.h"
#include "display_service.h"
#include "time_service.h"

bool system_init(void)
{
    if (!board_init())
    {
        return false;
    }

    time_service_init();

    if (!display_service_init())
    {
        return false;
    }

    return application_init();
}
