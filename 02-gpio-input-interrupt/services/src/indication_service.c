#include "indication_service.h"

#include "board_led.h"

void indication_service_init(void)
{
    board_led_set(false);
}

void indication_service_set(indication_id_t indication, bool enabled)
{
    if (indication == INDICATION_STATUS)
    {
        board_led_set(enabled);
    }
}

void indication_service_toggle(indication_id_t indication)
{
    if (indication == INDICATION_STATUS)
    {
        board_led_toggle();
    }
}
