#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "mcal_gpio.h"

#define BOARD_ADC_INPUT_PORT MCAL_GPIO_PORT_A
#define BOARD_ADC_INPUT_PIN  (0U)

#define BOARD_STATUS_LED_PORT       MCAL_GPIO_PORT_C
#define BOARD_STATUS_LED_PIN        (13U)
#define BOARD_STATUS_LED_ACTIVE_LOW (1U)

#endif
