#include "button_service.h"

#include <stdint.h>

#include "board_button.h"
#include "service_config.h"
#include "time_service.h"

static bool g_debounce_pending;
static uint32_t g_debounce_started_ms;

void button_service_init(void)
{
    g_debounce_pending = false;
    g_debounce_started_ms = time_service_now_ms();

    /*
     * Discard an EXTI event that may have been latched while the board
     * resources were being initialized.
     */
    (void)board_button_take_press_event();
}

bool button_service_take_press(void)
{
    if (board_button_take_press_event())
    {
        g_debounce_pending = true;
        g_debounce_started_ms = time_service_now_ms();
    }

    if (!g_debounce_pending)
    {
        return false;
    }

    if (time_service_elapsed_ms(g_debounce_started_ms) <
        BUTTON_SERVICE_DEBOUNCE_TIME_MS)
    {
        return false;
    }

    g_debounce_pending = false;
    return board_button_is_pressed();
}
