#include "board.h"
#include "board_can.h"
#include "board_config.h"
#include "board_led.h"
#include "board_timebase.h"
#include "mcal_rcc.h"

static board_clock_source_t g_clock_source = BOARD_CLOCK_SOURCE_HSI_FALLBACK;

bool board_init(void)
{
    if (mcal_rcc_configure_hse_pll(BOARD_HSE_FREQUENCY_HZ,
                                    BOARD_TARGET_CLOCK_HZ) == MCAL_RCC_CLOCK_OK) {
        g_clock_source = BOARD_CLOCK_SOURCE_HSE_PLL;
    } else {
        mcal_rcc_use_hsi();
        g_clock_source = BOARD_CLOCK_SOURCE_HSI_FALLBACK;
    }
    if (!board_led_init()) { return false; }
    board_led_set(false);
    if (!board_timebase_init(mcal_rcc_get_system_clock_hz())) { return false; }
    return board_can_init(mcal_rcc_get_apb1_clock_hz());
}

board_clock_source_t board_get_clock_source(void) { return g_clock_source; }
