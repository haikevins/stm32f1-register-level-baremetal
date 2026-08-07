#include "system.h"

#include "application.h"
#include "board.h"
#include "pwm_service.h"
#include "time_service.h"

bool system_init(void)
{
    if (!board_init())
    {
        return false;
    }

    time_service_init();
    pwm_service_init();
    application_init();
    return true;
}
