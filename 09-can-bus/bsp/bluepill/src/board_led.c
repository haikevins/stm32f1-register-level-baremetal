#include "board_led.h"

#include "board_pins.h"
#include "mcal_gpio.h"

bool board_led_init(void)
{
    return mcal_gpio_configure(
        BOARD_STATUS_LED_PORT,
        BOARD_STATUS_LED_PIN,
        MCAL_GPIO_MODE_OUTPUT_PP_2MHZ,
        MCAL_GPIO_LEVEL_HIGH);
}

void board_led_set(bool active)
{
#if BOARD_STATUS_LED_ACTIVE_LOW
    mcal_gpio_write(
        BOARD_STATUS_LED_PORT,
        BOARD_STATUS_LED_PIN,
        active ? MCAL_GPIO_LEVEL_LOW : MCAL_GPIO_LEVEL_HIGH);
#else
    mcal_gpio_write(
        BOARD_STATUS_LED_PORT,
        BOARD_STATUS_LED_PIN,
        active ? MCAL_GPIO_LEVEL_HIGH : MCAL_GPIO_LEVEL_LOW);
#endif
}

void board_led_toggle(void)
{
    mcal_gpio_toggle(BOARD_STATUS_LED_PORT, BOARD_STATUS_LED_PIN);
}
