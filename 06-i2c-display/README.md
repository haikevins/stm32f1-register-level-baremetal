# 06-i2c-display — I2C1 + SSD1306 OLED 128x64

## 1. Learning Objectives

This example introduces an external display and an ECUAL transport boundary.

You will learn:

- PB6/PB7 I2C1 register-level setup;
- open-drain GPIO;
- standard/fast-mode I2C timing calculation;
- bounded polling without I2C interrupts;
- SSD1306 command/data framing;
- framebuffer drawing;
- startup delay while global IRQ is disabled;
- Service composition of BSP transport and ECUAL driver.

## 2. Hardware and Wiring

```text
Blue Pill      SSD1306 module
-----------------------------
GND        ---> GND
3.3V       ---> VCC
PB6        ---> SCL
PB7        ---> SDA
```

Default 7-bit address:

```text
0x3C
```

Change to `0x3D` only if the actual module uses the alternate address.

I2C requires pull-ups to 3.3 V.

## 3. Expected Display

The demo renders:

```text
STM32F103
I2C SSD1306

UPTIME <seconds>
SECONDS

[progress bar]
```

The progress bar fills and empties continuously.

## 4. Compile-Time Configuration

```c
#define BOARD_TIMEBASE_HZ               (1000UL)
#define BOARD_DISPLAY_I2C_ADDRESS_7BIT  (0x3CU)
#define BOARD_DISPLAY_I2C_CLOCK_HZ      (400000UL)
#define BOARD_DISPLAY_POWER_ON_DELAY_MS (100UL)

#define DISPLAY_DEMO_UPDATE_PERIOD_MS   (100UL)
#define DISPLAY_DEMO_PROGRESS_STEP      (2U)

#define MCAL_I2C_POLL_TIMEOUT_CYCLES    (500000UL)
```

## 5. Pin Configuration

PB6 and PB7 are configured as alternate-function open-drain:

```text
PB6 -> I2C1_SCL
PB7 -> I2C1_SDA
```

Open-drain allows multiple I2C devices to share the bus safely.

## 6. I2C Clock — Normal 72 MHz System Clock

At normal clock:

```text
PCLK1 = 36 MHz
requested bus = 400 kHz
fast mode, duty 2
```

The MCAL computes:

```text
CCR = ceil(36 MHz / (3 * 400 kHz)) = 30
actual SCL = 400 kHz
TRISE = 11
```

The divider rounds conservatively so SCL does not exceed the requested rate.

## 7. HSI Fallback

With 8 MHz HSI fallback:

```text
PCLK1 = 8 MHz
CCR = ceil(8 MHz / (3 * 400 kHz)) = 7
actual SCL ~= 381 kHz
TRISE = 3
```

The bus remains valid without assuming the 72 MHz clock succeeded.

## 8. I2C Initialization Sequence

MCAL:

1. enables I2C1 clock;
2. resets I2C1;
3. writes CR2 frequency field;
4. configures OAR1;
5. calculates CCR;
6. calculates TRISE;
7. clears error state;
8. enables the peripheral;
9. verifies BUSY is clear.

BSP configures PB6/PB7 before calling MCAL I2C init.

## 9. Write Transaction

A display write uses:

```text
wait bus idle
    |
START
    |
wait SB
    |
send address + W
    |
wait ADDR
    |
read SR1 then SR2 to clear ADDR
    |
send SSD1306 control byte
    |
send payload
    |
wait BTF
    |
STOP
```

All polling is bounded.

## 10. SSD1306 Control Bytes

BSP prepends:

```text
0x00 -> command mode
0x40 -> data mode
```

The ECUAL driver only asks for command/data transport.

## 11. ECUAL Transport Design

The Display Service composes:

```text
SSD1306 transport
    |
    +--> board_display_bus_write
    +--> board_display_bus_delay_ms
```

The SSD1306 driver does not include BSP, MCAL, or STM32 register headers.

## 12. Power-On Delay

Global interrupts are disabled during `system_init()`.

Therefore the SSD1306 100 ms startup delay cannot rely on SysTick.

The board transport uses a conservative MCAL busy delay based on CPU NOP loops
for this hardware-settling phase.

This is intentionally limited to initialization.

## 13. SSD1306 Initialization

The ECUAL sends a fixed initialization command sequence covering:

- display off;
- oscillator/clock;
- multiplex;
- offset/start line;
- charge pump;
- horizontal addressing;
- remap/scan direction;
- COM pins;
- contrast;
- pre-charge;
- VCOMH;
- normal display;
- scroll off;
- display on.

Then it clears and presents the framebuffer.

## 14. Framebuffer Layout

Display:

```text
128 x 64
```

Pages:

```text
64 / 8 = 8
```

Framebuffer:

```text
128 * 8 = 1024 bytes
```

Pixel addressing:

```text
index = x + (y / 8) * 128
mask  = 1 << (y % 8)
```

## 15. Text Renderer

The ECUAL includes a compact 5x7 font for:

- space;
- punctuation used by the demo;
- digits;
- uppercase A-Z.

Characters are drawn pixel-by-pixel into the framebuffer.

## 16. Progress Bar

The progress-bar helper:

- draws a border;
- clamps percentage;
- computes fill width;
- sets/clears interior pixels.

Application only provides percentage and geometry.

## 17. Display Update

The driver sets:

```text
columns 0..127
pages 0..7
```

then sends the full 1024-byte framebuffer.

This is simple and deterministic, though not bandwidth-optimal.

## 18. Error Handling

MCAL detects I2C error flags including:

```text
BERR
ARLO
AF
OVR
TIMEOUT
```

A failed display present increments:

```text
application_display_error_count
```

and marks the display non-operational.

## 19. Interrupt Policy

I2C1 interrupts are not used.

The only runtime interrupt is SysTick.

I2C transfers use bounded polling.

## 20. Architecture

```text
Application
    |
Display Service
   / \
  /   \
SSD1306 ECUAL   Board Display Bus
                    |
                MCAL I2C/GPIO
                    |
               Platform Device
```

## Build, Flash, and Debug

```bash
make check-layers
make clean
make
make flash
```

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

Useful globals:

```gdb
p application_display_progress_percent
p application_display_update_count
p application_display_error_count
```

## 21. Test Procedure

1. Verify 3.3 V/GND.
2. Verify PB6/PB7.
3. Verify pull-ups.
4. Flash firmware.
5. Confirm text.
6. Confirm uptime increments.
7. Confirm progress bar moves.
8. Inspect update/error counters.
9. Measure SCL if a logic analyzer is available.

## 22. Troubleshooting

### OLED Is Completely Black

Check power, address, pull-ups, wiring, and initialization failure.

### I2C BUSY Is Always Set

SDA/SCL may be held low or a previous transaction may be incomplete.

Inspect physical line levels.

### Text Is Shifted or Inverted

Check controller compatibility, remap/scan-direction commands, and geometry.

### Update Error Appears After Running

Check signal quality, supply stability, pull-ups, and MCAL error flags.

## 23. Extension Exercises

- partial framebuffer updates;
- more glyphs;
- bitmap rendering;
- reset pin support;
- bus recovery;
- a second I2C device;
- asynchronous display update Service.

## 24. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
