# Porting guide

Board-specific assumptions are isolated under `bsp/bluepill`:

- SPI1 SCK: PA5
- SPI1 MISO: PA6
- SPI1 MOSI: PA7
- W25Q64 chip select: PA4
- status LED: PC13, active low

To port to another board, change the board pin mapping and bus composition.
The Application, Memory Service, and W25Q64 ECUAL can remain unchanged as
long as the BSP supplies the same transport callbacks.
