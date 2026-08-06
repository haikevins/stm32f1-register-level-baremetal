#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TARGET_CLOCK_HZ   (72000000UL)
#define BOARD_TIMEBASE_HZ       (1000UL)

#if BOARD_TIMEBASE_HZ != 1000UL
#error "The example time service currently expects a 1 kHz board timebase."
#endif

#endif
