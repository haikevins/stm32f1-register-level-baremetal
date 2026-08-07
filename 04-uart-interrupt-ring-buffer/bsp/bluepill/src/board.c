#include "board.h"

#include "board_config.h"
#include "board_uart.h"
#include "mcal_rcc.h"

static board_clock_source_t g_clock_source = BOARD_CLOCK_SOURCE_HSI_FALLBACK;

bool board_init(void)
{
    const mcal_rcc_clock_status_t clock_status =
        mcal_rcc_configure_hse_pll(BOARD_HSE_FREQUENCY_HZ,
                                   BOARD_TARGET_CLOCK_HZ);

    if (clock_status == MCAL_RCC_CLOCK_OK)
    {
        g_clock_source = BOARD_CLOCK_SOURCE_HSE_PLL;
    }
    else
    {
        mcal_rcc_use_hsi();
        g_clock_source = BOARD_CLOCK_SOURCE_HSI_FALLBACK;
    }

    return board_uart_init(mcal_rcc_get_apb2_clock_hz());
}

board_clock_source_t board_get_clock_source(void)
{
    return g_clock_source;
}
