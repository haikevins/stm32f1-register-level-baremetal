# STM32F1 Register-Level Bare-Metal Examples

A collection of self-contained examples for the **STM32F103C8T6 Blue Pill**, implemented directly at register level with C11 and ARM assembly.

These examples are built from the project skeleton maintained on the repository's `template` branch. Each example is an independent firmware project with its own Makefile, startup code, linker script, layered architecture, and documentation.

## Design Principles

- No STM32 HAL
- No STM32 LL
- No Standard Peripheral Library
- No libopencm3
- No Arduino Core
- No RTOS
- No dynamic memory allocation
- Direct peripheral register access through project-owned MCAL drivers
- Strict downward dependencies between architecture layers
- Non-blocking application logic whenever practical

## Available Examples

| Directory | Description | Main peripherals |
|---|---|---|
| [`01-blink-led`](01-blink-led/) | Non-blocking blink of the Blue Pill PC13 status LED | RCC, GPIO, SysTick |

More examples can be added as separate numbered directories:

```text
02-gpio-input-interrupt/
03-uart-polling/
04-uart-interrupt-ring-buffer/
05-timer-pwm/
06-spi-display/
07-i2c-sensor/
08-adc-dma/
09-can-loopback/
```

## Clone the Examples Branch

```bash
git clone \
    --branch examples \
    --single-branch \
    https://github.com/haikevins/stm32f1-register-level-baremetal.git \
    stm32f1-examples

cd stm32f1-examples
```

## Build an Example

Enter the example directory and run `make`:

```bash
cd 01-blink-led
make
```

Generated files are written to the example's local `build/` directory:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

## Flash

Connect the Blue Pill to an ST-Link through SWD:

```bash
make flash
```

## Debug

Start OpenOCD in the first terminal:

```bash
make debug-server
```

Start GDB in a second terminal:

```bash
make debug
```

Depending on the local toolchain, the Makefile uses `arm-none-eabi-gdb` or `gdb-multiarch`.

## Architecture

The examples follow this dependency direction:

```text
Application
    |
    v
Services
    |
    v
BSP / ECU Abstraction
    |
    v
MCAL
    |
    v
Device / Architecture
    |
    v
Hardware
```

Each example includes an architecture checker:

```bash
make check-layers
```

A higher layer must not include or call directly into a lower-level implementation that bypasses the intended abstraction boundary.

## Creating a New Example

Use the `template` branch as the clean starting point rather than copying unrelated example logic:

```bash
git clone \
    --branch template \
    --single-branch \
    https://github.com/haikevins/stm32f1-register-level-baremetal.git \
    02-my-example
```

Then:

1. Implement the required MCAL drivers.
2. Add board-specific mappings in the BSP.
3. Add external-device drivers in ECUAL when needed.
4. Add Services that expose hardware-independent APIs.
5. Write the example behavior in the Application layer.
6. Connect module initialization in `system/system_init.c`.
7. Document the hardware wiring and expected behavior in the example README.
8. Run `make check-layers` and `make` before committing.

## Prerequisites

- GNU Arm Embedded Toolchain
- GNU Make
- Python 3
- OpenOCD
- `arm-none-eabi-gdb` or `gdb-multiarch`
- ST-Link or a compatible SWD probe
- STM32F103C8T6 Blue Pill board

## License

The examples are distributed under the MIT License. Refer to the `LICENSE` file inside each example where applicable.
