#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_HSE_FREQUENCY_HZ       (8000000UL)
#define BOARD_TARGET_CLOCK_HZ         (72000000UL)

#define BOARD_TIMEBASE_HZ             (1000UL)

/*
 * Conservative breadboard SPI rate. With PCLK2 = 72 MHz this selects
 * prescaler /16, giving a 4.5 MHz SPI clock.
 */
#define BOARD_MEMORY_SPI_MAX_HZ       (5000000UL)
#define BOARD_MEMORY_POWER_ON_DELAY_MS (10UL)

#if BOARD_TIMEBASE_HZ != 1000UL
#error "The example time service expects a 1 kHz board timebase."
#endif

#if BOARD_MEMORY_SPI_MAX_HZ == 0UL
#error "BOARD_MEMORY_SPI_MAX_HZ must be non-zero."
#endif

#endif
