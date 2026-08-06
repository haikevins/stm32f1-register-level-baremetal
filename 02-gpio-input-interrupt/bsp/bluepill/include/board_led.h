#ifndef BOARD_LED_H
#define BOARD_LED_H

#include <stdbool.h>

bool board_led_init(void);
void board_led_set(bool enabled);
void board_led_toggle(void);

#endif
