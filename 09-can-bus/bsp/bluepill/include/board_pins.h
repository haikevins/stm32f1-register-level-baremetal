#ifndef BOARD_PINS_H
#define BOARD_PINS_H
#include "mcal_gpio.h"
#define BOARD_STATUS_LED_PORT       MCAL_GPIO_PORT_C
#define BOARD_STATUS_LED_PIN        (13U)
#define BOARD_STATUS_LED_ACTIVE_LOW (1U)
#define BOARD_CAN_RX_PORT           MCAL_GPIO_PORT_B
#define BOARD_CAN_RX_PIN            (8U)
#define BOARD_CAN_TX_PORT           MCAL_GPIO_PORT_B
#define BOARD_CAN_TX_PIN            (9U)
#endif
