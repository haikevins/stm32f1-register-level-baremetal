#ifndef MCAL_CAN_H
#define MCAL_CAN_H
#include <stdbool.h>
#include <stdint.h>
#include "can_types.h"
typedef enum {
    MCAL_CAN_TX_IDLE = 0,
    MCAL_CAN_TX_PENDING,
    MCAL_CAN_TX_OK,
    MCAL_CAN_TX_ERROR
} mcal_can_tx_state_t;
bool mcal_can_init(uint32_t apb1_hz, uint32_t bitrate_hz,
                   uint16_t filter_id, uint32_t timeout);
bool mcal_can_transmit(const can_frame_t *frame);
mcal_can_tx_state_t mcal_can_poll_tx(void);
bool mcal_can_pop_fifo0(can_frame_t *frame);
uint32_t mcal_can_get_esr(void);
#endif
