#include "system.h"

#include "application.h"
#include "board.h"
#include "serial_service.h"

bool system_init(void)
{
    if (!board_init())
    {
        return false;
    }

    serial_service_init();
    application_init();
    return true;
}
