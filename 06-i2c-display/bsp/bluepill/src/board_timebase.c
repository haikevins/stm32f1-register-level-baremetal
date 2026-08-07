#include "board_timebase.h"

#include "board_config.h"
#include "mcal_systick.h"

bool board_timebase_init(uint32_t core_clock_hz)
{
    return mcal_systick_init(core_clock_hz, BOARD_TIMEBASE_HZ);
}

uint32_t board_timebase_now_ms(void)
{
    return mcal_systick_get_ticks();
}
