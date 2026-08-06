#include "application.h"

#include "button_service.h"
#include "indication_service.h"

void application_init(void)
{
    indication_service_set(INDICATION_STATUS, false);
}

void application_process(void)
{
    if (button_service_take_press())
    {
        indication_service_toggle(INDICATION_STATUS);
    }
}
