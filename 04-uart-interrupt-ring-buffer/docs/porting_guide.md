# Porting guide

To move the example to another STM32F1 board:

1. Update the clock values in `config/board_config.h`.
2. Change TX/RX mapping in `bsp/bluepill/include/board_pins.h`.
3. Update `board_uart.c` if another USART instance is required.
4. Add the new USART base address, clock-enable bit and register mapping to
   the platform layer.
5. Supply the correct peripheral-bus clock to `mcal_usart_init()`.

The Application and Serial Service should remain unchanged.
