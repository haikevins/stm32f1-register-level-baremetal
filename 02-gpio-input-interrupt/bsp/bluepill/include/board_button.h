#ifndef BOARD_BUTTON_H
#define BOARD_BUTTON_H

#include <stdbool.h>

bool board_button_init(void);
bool board_button_is_pressed(void);
bool board_button_take_press_event(void);

#endif
