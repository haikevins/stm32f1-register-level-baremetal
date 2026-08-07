# 06-i2c-display

Register-level bare-metal SSD1306 OLED example for the STM32F103C8T6 Blue Pill.

## Hardware

Four-pin SSD1306-compatible 128x64 I2C OLED:

| Blue Pill | OLED |
|---|---|
| GND | GND |
| 3.3V | VCC |
| PB6 / I2C1_SCL | SCL |
| PB7 / I2C1_SDA | SDA |

Most four-pin OLED modules already include I2C pull-up resistors. If yours does
not, add approximately 4.7 kOhm pull-ups from SCL and SDA to 3.3 V.

The default 7-bit address is `0x3C`. Change
`BOARD_DISPLAY_I2C_ADDRESS_7BIT` to `0x3D` when required.

## Behavior

The display shows:

- `STM32F103`
- `I2C SSD1306`
- uptime in seconds
- a progress bar that repeatedly fills and empties

The framebuffer is 1024 bytes (`128 * 64 / 8`) and is statically allocated.

## Architecture

```text
Application
    |
Display Service
   / \
  /   \
SSD1306 ECUAL   Board Display Bus
                  |
                MCAL I2C + GPIO
                  |
              STM32F103 registers
```

The SSD1306 ECUAL does not include BSP or MCAL headers. The service supplies
transport callbacks implemented by the BSP.

## I2C configuration

- Peripheral: I2C1
- SCL: PB6
- SDA: PB7
- Address: 0x3C
- Requested bus rate: 400 kHz
- Transfer style: polling
- I2C interrupts: not used

At the normal 72 MHz system clock, PCLK1 is 36 MHz and fast-mode CCR is 30.
With the 8 MHz HSI fallback, the divider is rounded upward so SCL remains at
or below the requested 400 kHz.

## Build

```sh
make check-layers
make clean
make
make flash
```

`system_idle()` intentionally executes `NOP` rather than `WFI`, which keeps
SWD attach behavior predictable for ST-Link adapters without NRST wiring.
