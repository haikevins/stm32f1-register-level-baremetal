#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "mcal_gpio.h"

/*
 * W25Q64 six-pin SPI module:
 *
 *   PA5  SPI1_SCK  -> CLK
 *   PA6  SPI1_MISO <- D1 / DO / IO1
 *   PA7  SPI1_MOSI -> D0 / DI / IO0
 *   PA4            -> CS
 *
 * Power:
 *   Blue Pill 3.3V -> VCC
 *   Blue Pill GND  -> GND
 */
#define BOARD_MEMORY_SPI_PORT   MCAL_GPIO_PORT_A
#define BOARD_MEMORY_SCK_PIN    (5U)
#define BOARD_MEMORY_MISO_PIN   (6U)
#define BOARD_MEMORY_MOSI_PIN   (7U)
#define BOARD_MEMORY_CS_PIN     (4U)

#define BOARD_STATUS_LED_PORT       MCAL_GPIO_PORT_C
#define BOARD_STATUS_LED_PIN        (13U)
#define BOARD_STATUS_LED_ACTIVE_LOW (1U)

#endif
