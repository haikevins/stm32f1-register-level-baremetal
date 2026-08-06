#include "board_button.h"

#include "board_pins.h"
#include "mcal_exti.h"
#include "mcal_gpio.h"

bool board_button_init(void)
{
    if (!mcal_gpio_configure(BOARD_USER_BUTTON_PORT,
                             BOARD_USER_BUTTON_PIN,
                             MCAL_GPIO_MODE_INPUT_PULL,
                             MCAL_GPIO_LEVEL_HIGH))
    {
        return false;
    }

    return mcal_exti_configure_line(
        BOARD_USER_BUTTON_EXTI_LINE,
        BOARD_USER_BUTTON_PORT,
        MCAL_EXTI_TRIGGER_FALLING,
        BOARD_USER_BUTTON_IRQ_PRIORITY);
}

bool board_button_is_pressed(void)
{
    return mcal_gpio_read(BOARD_USER_BUTTON_PORT,
                          BOARD_USER_BUTTON_PIN) ==
           BOARD_USER_BUTTON_ACTIVE_LEVEL;
}

bool board_button_take_press_event(void)
{
    return mcal_exti_take_event(BOARD_USER_BUTTON_EXTI_LINE);
}
