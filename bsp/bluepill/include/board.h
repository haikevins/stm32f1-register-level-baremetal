#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

/*
 * Initialize logical resources provided by the selected board.
 * Add board modules separately; do not put product behavior here.
 */
bool board_init(void);

#endif
