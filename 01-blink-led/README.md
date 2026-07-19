# Example 01: Non-Blocking LED Blink

A register-level bare-metal example for the **STM32F103C8T6 Blue Pill**.

After flashing, the onboard status LED connected to **PC13** toggles every **500 ms**. The implementation is non-blocking: the application does not use a busy-wait delay loop.

## What This Example Demonstrates

- Custom Cortex-M3 startup code and interrupt vector table
- A project-owned linker script for the STM32F103C8T6
- Runtime initialization of `.data` and `.bss`
- Register-level RCC clock configuration
- Register-level GPIO output control
- A 1 kHz Cortex-M3 SysTick timebase
- Non-blocking periodic application behavior
- Strict layered architecture
- Separation between logical indications and physical board pins
- HSE-to-PLL clock configuration with an HSI fallback

No STM32 HAL, LL, SPL, libopencm3, Arduino Core, or RTOS is used.

## Expected Behavior

The Blue Pill onboard LED is connected to PC13 and is active-low.

| Setting | Value |
|---|---|
| LED pin | PC13 |
| Active level | Low |
| Toggle period | 500 ms |
| Preferred system clock | 72 MHz |
| Preferred clock source | 8 MHz HSE multiplied by 9 |
| Fallback clock source | 8 MHz HSI |
| Timebase | 1 kHz SysTick |

Because the LED is active-low:

```text
PC13 = 0 -> LED on
PC13 = 1 -> LED off
```

## Layered Execution Flow

LED control follows this path:

```text
Application
    |
    v
Indication Service
    |
    v
Board LED
    |
    v
MCAL GPIO
    |
    v
GPIOC registers
```

Timekeeping follows this path:

```text
Application
    |
    v
Time Service
    |
    v
Board Timebase
    |
    v
MCAL SysTick
    |
    v
Cortex-M3 SysTick registers
```

The application never accesses GPIO, SysTick, RCC, or STM32 register definitions directly.

## Application Logic

The application stores the timestamp of the previous toggle and checks whether the configured period has elapsed:

```c
void application_process(void)
{
    if (time_service_periodic_due(
            &g_last_blink_ms,
            APPLICATION_BLINK_PERIOD_MS))
    {
        indication_service_toggle(INDICATION_STATUS);
    }
}
```

This keeps the super-loop available for additional tasks.

The blink period is configured in:

```text
config/application_config.h
```

```c
#define APPLICATION_BLINK_PERIOD_MS (500UL)
```

## Initialization Order

`system/system_init.c` acts as the composition root:

```text
board_init()
    |
    +--> Configure HSE + PLL, or fall back to HSI
    +--> Initialize the PC13 status LED
    +--> Initialize the 1 kHz SysTick timebase
    |
    v
time_service_init()
    |
    v
indication_service_init()
    |
    v
event_service_init()
    |
    v
application_init()
```

Application behavior does not belong in `system/`; that directory only connects and initializes modules.

## Hardware

Required hardware:

- STM32F103C8T6 Blue Pill
- ST-Link or another OpenOCD-compatible SWD probe
- USB or another suitable 5 V / 3.3 V board power source
- SWD wiring

Typical ST-Link wiring:

| ST-Link | Blue Pill |
|---|---|
| SWDIO | PA13 |
| SWCLK | PA14 |
| GND | GND |
| 3.3 V reference | 3.3 V |

Ensure that the target and debug probe share a common ground.

## Prerequisites

- GNU Arm Embedded Toolchain
- GNU Make
- Python 3
- OpenOCD
- `arm-none-eabi-gdb` or `gdb-multiarch`

Check the tools:

```bash
arm-none-eabi-gcc --version
make --version
openocd --version
```

## Clone

Clone only the `examples` branch:

```bash
git clone \
    --branch examples \
    --single-branch \
    https://github.com/haikevins/stm32f1-register-level-baremetal.git \
    stm32f1-examples

cd stm32f1-examples/01-blink-led
```

## Build

```bash
make
```

Generated files:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

Additional build commands:

```bash
make check-layers
make size
make tree
make clean
```

## Flash

Connect the ST-Link and run:

```bash
make flash
```

Erase the MCU:

```bash
make erase
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

The debugger loads `build/firmware.elf`, connects to OpenOCD through the configuration in `tools/gdb/debug.gdb`, and stops at the configured breakpoint.

## Configuration

Application timing:

```text
config/application_config.h
```

Board clock and timebase:

```text
config/board_config.h
```

Board LED mapping:

```text
bsp/bluepill/include/board_pins.h
```

The default configuration is:

```c
#define APPLICATION_BLINK_PERIOD_MS (500UL)

#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TARGET_CLOCK_HZ  (72000000UL)
#define BOARD_TIMEBASE_HZ       (1000UL)
```

## Project Structure

```text
app/                         Example behavior
services/                    Time, indication, and event services
bsp/bluepill/                Board clock, LED, and timebase abstraction
mcal/                        RCC, GPIO, and SysTick drivers
platform/arch/cortex-m3/     Cortex-M3 register definitions
platform/device/stm32f103xb/ STM32F103 register definitions
system/                      Composition root and main loop
startup/                     Reset handler and vector table
linker/                      Memory layout
config/                      Compile-time settings
tools/                       OpenOCD, GDB, and dependency checks
```

## Layer Rules

- Application may depend on Services and portable Common types.
- Services may depend on BSP, ECUAL, and Common.
- BSP and ECUAL may depend on MCAL and Common.
- MCAL may depend on Device, Architecture, and low-level Common code.
- Device and Architecture code must not depend on any upper layer.
- MCAL interrupt handlers must not call Application or Service callbacks.

Validate these rules with:

```bash
make check-layers
```

## Extending the Example

Suitable next steps include:

- Change the blink period at runtime
- Add a push button through EXTI
- Add a second software timer
- Report the selected clock source over UART
- Replace the LED behavior with an application state machine
- Add a watchdog service

Keep new functionality in the correct layer rather than accessing registers directly from `application.c`.

## License

This example is licensed under the MIT License. See [`LICENSE`](LICENSE) for details.
