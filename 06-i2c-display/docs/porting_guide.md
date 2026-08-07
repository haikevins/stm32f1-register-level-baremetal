# Porting guide

To move this example to another STM32F1 board:

1. Map the I2C SCL/SDA pins in `bsp/.../board_pins.h`.
2. Configure the pins as alternate-function open-drain in the BSP.
3. Select the desired I2C peripheral in `board_display_bus.c`.
4. Extend `mcal_i2c.c` and the device register map if another I2C instance is used.
5. Pass the correct APB peripheral clock from the RCC layer.
6. Update the SSD1306 7-bit address in `config/board_config.h` when needed.
7. Keep the service/application API unchanged.

The current Blue Pill mapping is PB6 -> I2C1_SCL and PB7 -> I2C1_SDA,
with no AFIO remap.
