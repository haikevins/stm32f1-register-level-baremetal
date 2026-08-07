#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_HSE_FREQUENCY_HZ             (8000000UL)
#define BOARD_TARGET_CLOCK_HZ               (72000000UL)

#define BOARD_TIMEBASE_HZ                   (1000UL)

/*
 * SSD1306 four-pin I2C modules commonly use 7-bit address 0x3C.
 * Change to 0x3D if your module is strapped for the alternate address.
 */
#define BOARD_DISPLAY_I2C_ADDRESS_7BIT      (0x3CU)
#define BOARD_DISPLAY_I2C_CLOCK_HZ          (400000UL)
#define BOARD_DISPLAY_POWER_ON_DELAY_MS     (100UL)

#if BOARD_TIMEBASE_HZ != 1000UL
#error "The display example expects a 1 kHz board timebase."
#endif

#if BOARD_DISPLAY_I2C_ADDRESS_7BIT > 0x7FUL
#error "BOARD_DISPLAY_I2C_ADDRESS_7BIT must be a 7-bit address."
#endif

#if (BOARD_DISPLAY_I2C_CLOCK_HZ == 0UL) || \
    (BOARD_DISPLAY_I2C_CLOCK_HZ > 400000UL)
#error "BOARD_DISPLAY_I2C_CLOCK_HZ must be in the range 1..400000."
#endif

#endif
