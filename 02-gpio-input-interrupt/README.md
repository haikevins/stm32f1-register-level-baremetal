# Example 02: GPIO Input Interrupt

A register-level bare-metal example for the **STM32F103C8T6 Blue Pill**.

Pressing an external push button connected between **PA0 and GND** toggles the
onboard active-low LED on **PC13**. PA0 uses the STM32 internal pull-up, EXTI0
detects the falling edge, and a 30 ms debounce check runs outside the interrupt
handler.

No STM32 HAL, LL, SPL, libopencm3, Arduino Core, or RTOS is used.

## What This Example Demonstrates

- Register-level GPIO input with an internal pull-up
- AFIO external-interrupt line routing
- EXTI falling-edge configuration
- Cortex-M3 NVIC priority and interrupt enable registers
- A minimal EXTI ISR that only acknowledges hardware and records an event
- Thread-mode debounce using the SysTick time service
- Strict layered architecture
- Active-low Blue Pill LED control
- Debug-safe `NOP` idle behavior for ST-Link probes without NRST wiring

## Hardware

Connect a normally-open push button:

```text
PA0 ---- push button ---- GND
```

The internal pull-up keeps PA0 high while the button is released.

| Resource | Configuration |
|---|---|
| Button pin | PA0 |
| Input mode | Pull-up |
| Active level | Low |
| EXTI line | EXTI0 |
| Trigger | Falling edge |
| Debounce | 30 ms |
| LED pin | PC13 |
| LED active level | Low |

Do not connect PA0 directly to 3.3 V and GND at the same time.

## Execution Flow

```text
Button press
    |
    v
PA0 falling edge
    |
    v
EXTI0_IRQHandler
    |
    +--> Clear EXTI pending flag
    +--> Set MCAL-owned event bit
    |
    v
Button Service
    |
    +--> Wait 30 ms in thread mode
    +--> Confirm PA0 is still low
    |
    v
Application
    |
    v
Indication Service
    |
    v
PC13 LED toggle
```

The interrupt handler does not debounce, toggle the LED, call a service, or
block.

## Layered Path

Button input:

```text
Application
    |
    v
Button Service
    |
    v
Board Button
    |
    v
MCAL GPIO + EXTI
    |
    v
GPIOA / AFIO / EXTI / NVIC registers
```

LED output:

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

## Interrupt Ownership

The EXTI handler belongs to `mcal/src/mcal_exti.c`, the lowest module that owns
the peripheral:

```c
void EXTI0_IRQHandler(void)
{
    record_pending_lines(UINT32_C(1) << 0U);
}
```

`record_pending_lines()` clears the write-one-to-clear EXTI pending flag and
sets a bit in an MCAL-owned static event mask. The upper layers poll that event
during normal thread-mode execution.

## Debounce

The first falling edge starts or restarts a 30 ms debounce interval. When the
interval expires, the button service reads PA0 again:

```c
if (time_service_elapsed_ms(g_debounce_started_ms) <
    BUTTON_SERVICE_DEBOUNCE_TIME_MS)
{
    return false;
}

g_debounce_pending = false;
return board_button_is_pressed();
```

A press is delivered to the application only when PA0 remains low after the
debounce interval.

Configure the interval in:

```text
config/service_config.h
```

```c
#define BUTTON_SERVICE_DEBOUNCE_TIME_MS (30UL)
```

## Idle Behavior

This example intentionally uses:

```c
void system_idle(void)
{
    cortex_m3_nop();
}
```

instead of `WFI`. This keeps the MCU in Run mode and makes repeated SWD
attachment more reliable when the ST-Link wiring does not include the target
NRST pin. EXTI and SysTick still operate normally.

## Build

```bash
make check-layers
make clean
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

## Flash

```bash
make flash
```

## Debug

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

Useful GDB commands:

```gdb
break EXTI0_IRQHandler
continue
```

After the ISR breakpoint is hit:

```gdb
bt
continue
```

## Project Structure

```text
app/                         Toggle behavior
services/                    Button debounce, time, indication, events
bsp/bluepill/                PA0 button and PC13 LED mapping
mcal/                        GPIO, EXTI, RCC, SysTick
platform/arch/cortex-m3/     SysTick, NVIC and core registers
platform/device/stm32f103xb/ AFIO, EXTI, GPIO and RCC register maps
system/                      Composition root and super-loop
startup/                     Reset handler and vector table
config/                      Compile-time settings
tools/                       Build, flash, debug and layer checks
```

## Important Register Configuration

The project performs the equivalent register-level setup:

```text
RCC_APB2ENR.AFIOEN = 1
RCC_APB2ENR.IOPAEN = 1
GPIOA_CRL.CNF0/MODE0 = input pull-up
GPIOA_ODR.ODR0 = 1
AFIO_EXTICR1.EXTI0 = PA0
EXTI_FTSR.TR0 = 1
EXTI_IMR.MR0 = 1
NVIC_ISER.EXTI0 = 1
```

## License

This example is licensed under the MIT License.
