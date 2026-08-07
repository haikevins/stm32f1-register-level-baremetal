#include "board_display_bus.h"

#include "board_config.h"
#include "board_pins.h"
#include "mcal_delay.h"
#include "mcal_gpio.h"
#include "mcal_i2c.h"
#include "mcal_rcc.h"

#define SSD1306_I2C_CONTROL_COMMAND (0x00U)
#define SSD1306_I2C_CONTROL_DATA    (0x40U)

bool board_display_bus_init(uint32_t peripheral_clock_hz)
{
    if (!mcal_gpio_configure(BOARD_DISPLAY_I2C_PORT,
                             BOARD_DISPLAY_SCL_PIN,
                             MCAL_GPIO_MODE_AF_OD_50MHZ,
                             MCAL_GPIO_LEVEL_HIGH))
    {
        return false;
    }

    if (!mcal_gpio_configure(BOARD_DISPLAY_I2C_PORT,
                             BOARD_DISPLAY_SDA_PIN,
                             MCAL_GPIO_MODE_AF_OD_50MHZ,
                             MCAL_GPIO_LEVEL_HIGH))
    {
        return false;
    }

    return mcal_i2c_init(MCAL_I2C_INSTANCE_1,
                         peripheral_clock_hz,
                         BOARD_DISPLAY_I2C_CLOCK_HZ);
}

bool board_display_bus_write(bool data_mode,
                             const uint8_t *data,
                             size_t length)
{
    const uint8_t control_byte =
        data_mode
            ? SSD1306_I2C_CONTROL_DATA
            : SSD1306_I2C_CONTROL_COMMAND;

    return mcal_i2c_master_write_prefixed(
        MCAL_I2C_INSTANCE_1,
        BOARD_DISPLAY_I2C_ADDRESS_7BIT,
        control_byte,
        data,
        length);
}

void board_display_bus_delay_ms(uint32_t delay_ms)
{
    mcal_delay_busy_ms(mcal_rcc_get_system_clock_hz(), delay_ms);
}
