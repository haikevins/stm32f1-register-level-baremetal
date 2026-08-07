# Porting guide

To move this example to another STM32F1 board:

1. Map a timer channel pin in `bsp/.../board_pins.h`.
2. Configure the matching GPIO alternate-function mode in the BSP.
3. Select the timer instance/channel in `board_pwm.c`.
4. Extend `mcal_timer.c` and the device register map if another timer is used.
5. Supply the actual timer input clock from the RCC layer.
6. Keep the service/application API unchanged.

The current Blue Pill mapping is PA0 -> TIM2_CH1 with no AFIO remap.
