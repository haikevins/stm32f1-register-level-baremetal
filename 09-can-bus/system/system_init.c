#include "system.h"
#include "application.h"
#include "board.h"
#include "can_service.h"
#include "indication_service.h"
#include "time_service.h"
bool system_init(void)
{
    if (!board_init()) { return false; }
    time_service_init();
    indication_service_init();
    can_service_init();
    application_init();
    return true;
}
