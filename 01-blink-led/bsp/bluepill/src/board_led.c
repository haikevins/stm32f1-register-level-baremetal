#include "board_led.h"

#include "board_pins.h"
#include "mcal_gpio.h"

static mcal_gpio_level_t inactive_level(void)
{
    return (BOARD_STATUS_LED_ACTIVE_LEVEL == MCAL_GPIO_LEVEL_HIGH)
               ? MCAL_GPIO_LEVEL_LOW
               : MCAL_GPIO_LEVEL_HIGH;
}

bool board_led_init(void)
{
    return mcal_gpio_configure(BOARD_STATUS_LED_PORT,
                               BOARD_STATUS_LED_PIN,
                               MCAL_GPIO_MODE_OUTPUT_PP_2MHZ,
                               inactive_level());
}

void board_led_set(bool enabled)
{
    mcal_gpio_write(BOARD_STATUS_LED_PORT,
                    BOARD_STATUS_LED_PIN,
                    enabled ? BOARD_STATUS_LED_ACTIVE_LEVEL : inactive_level());
}

void board_led_toggle(void)
{
    mcal_gpio_toggle(BOARD_STATUS_LED_PORT, BOARD_STATUS_LED_PIN);
}
