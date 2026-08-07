#include "system.h"

#include "adc_service.h"
#include "application.h"
#include "board.h"
#include "indication_service.h"

bool system_init(void)
{
    if (!board_init())
    {
        return false;
    }

    adc_service_init();
    indication_service_init();
    application_init();

    return true;
}
