#ifndef BOARD_CAN_H
#define BOARD_CAN_H
#include <stdbool.h>
#include <stdint.h>
#include "can_types.h"
typedef enum {
    BOARD_CAN_TX_IDLE = 0,
    BOARD_CAN_TX_PENDING,
    BOARD_CAN_TX_OK,
    BOARD_CAN_TX_ERROR
} board_can_tx_state_t;
bool board_can_init(uint32_t apb1_clock_hz);
bool board_can_transmit(const can_frame_t *frame);
board_can_tx_state_t board_can_poll_tx(void);
bool board_can_take_received(can_frame_t *frame);
uint32_t board_can_get_overruns(void);
uint32_t board_can_get_error_status(void);
#endif
