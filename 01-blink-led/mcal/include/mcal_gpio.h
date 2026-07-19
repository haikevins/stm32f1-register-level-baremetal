#ifndef MCAL_GPIO_H
#define MCAL_GPIO_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MCAL_GPIO_PORT_A = 0,
    MCAL_GPIO_PORT_B,
    MCAL_GPIO_PORT_C,
    MCAL_GPIO_PORT_D,
    MCAL_GPIO_PORT_E
} mcal_gpio_port_t;

typedef enum
{
    MCAL_GPIO_MODE_INPUT_ANALOG = 0x0,
    MCAL_GPIO_MODE_OUTPUT_PP_10MHZ = 0x1,
    MCAL_GPIO_MODE_OUTPUT_PP_2MHZ = 0x2,
    MCAL_GPIO_MODE_OUTPUT_PP_50MHZ = 0x3,
    MCAL_GPIO_MODE_INPUT_FLOATING = 0x4,
    MCAL_GPIO_MODE_OUTPUT_OD_10MHZ = 0x5,
    MCAL_GPIO_MODE_OUTPUT_OD_2MHZ = 0x6,
    MCAL_GPIO_MODE_OUTPUT_OD_50MHZ = 0x7,
    MCAL_GPIO_MODE_INPUT_PULL = 0x8,
    MCAL_GPIO_MODE_AF_PP_10MHZ = 0x9,
    MCAL_GPIO_MODE_AF_PP_2MHZ = 0xA,
    MCAL_GPIO_MODE_AF_PP_50MHZ = 0xB,
    MCAL_GPIO_MODE_AF_OD_10MHZ = 0xD,
    MCAL_GPIO_MODE_AF_OD_2MHZ = 0xE,
    MCAL_GPIO_MODE_AF_OD_50MHZ = 0xF
} mcal_gpio_mode_t;

typedef enum
{
    MCAL_GPIO_LEVEL_LOW = 0,
    MCAL_GPIO_LEVEL_HIGH = 1
} mcal_gpio_level_t;

bool mcal_gpio_configure(mcal_gpio_port_t port,
                         uint8_t pin,
                         mcal_gpio_mode_t mode,
                         mcal_gpio_level_t initial_or_pull_level);
void mcal_gpio_write(mcal_gpio_port_t port,
                     uint8_t pin,
                     mcal_gpio_level_t level);
mcal_gpio_level_t mcal_gpio_read(mcal_gpio_port_t port, uint8_t pin);
void mcal_gpio_toggle(mcal_gpio_port_t port, uint8_t pin);

#endif
