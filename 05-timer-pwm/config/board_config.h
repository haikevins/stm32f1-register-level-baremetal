#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define BOARD_HSE_FREQUENCY_HZ  (8000000UL)
#define BOARD_TARGET_CLOCK_HZ    (72000000UL)

#define BOARD_TIMEBASE_HZ        (1000UL)

#define BOARD_PWM_TIMER_TICK_HZ  (1000000UL)
#define BOARD_PWM_FREQUENCY_HZ   (1000UL)

#if BOARD_TIMEBASE_HZ != 1000UL
#error "The example time service expects a 1 kHz board timebase."
#endif

#if BOARD_PWM_TIMER_TICK_HZ == 0UL
#error "BOARD_PWM_TIMER_TICK_HZ must be non-zero."
#endif

#if BOARD_PWM_FREQUENCY_HZ == 0UL
#error "BOARD_PWM_FREQUENCY_HZ must be non-zero."
#endif

#if (BOARD_PWM_TIMER_TICK_HZ % BOARD_PWM_FREQUENCY_HZ) != 0UL
#error "PWM timer tick must be an integer multiple of PWM frequency."
#endif

#endif
