# 02 GPIO input interrupt - Architecture

> **Focus:** ownership, dependency direction, initialization ordering, execution contexts, data lifetime, and failure invariants for `02-gpio-input-interrupt`.

[← Example README](../README.md) · [Porting guide →](porting_guide.md) · [Examples index](../../README.md) · [Root](../../../README.md)

## Table of contents

- [Architectural slice](#architectural-slice)
- [Composition and initialization](#composition-and-initialization)
- [Source map and exact composition](#source-map-and-exact-composition)
- [Resource ownership](#resource-ownership)
- [Data and control flow](#data-and-control-flow)
- [Concurrency model](#concurrency-model)
- [Failure model](#failure-model)
- [Invariants](#invariants)
- [Why this structure matters](#why-this-structure-matters)
- [References](#references)

## Architectural slice

The example uses the repository's full dependency vocabulary even when some directories are intentionally thin:

```text
app
 |
 v
services
 |\
 | +------> ecual (only when an external-device protocol is used)
 v
bsp/bluepill
 |
 v
mcal
 |\
 | +------> platform/arch
 v
platform/device
```

`system` is the composition root. It initializes the concrete board resources and services before handing control to the application.

`check_layers.py` mechanically checks local include direction. `system` is intentionally exempt from normal downward-only restrictions because it is the composition root that wires otherwise separated modules together.

The architecture is **procedural and statically composed**. There is no dependency-injection framework, heap, object registry, or RTOS task container. Dependencies are expressed through C calls and, for ECUAL transports, small function-pointer interfaces.

## Composition and initialization

The reset path establishes C runtime memory before any module initialization:

```text
vector table -> Reset_Handler -> runtime_init
                              -> .data copy
                              -> .bss clear
                              -> main
```

`main()` disables global interrupts, calls `system_init()`, enables them only on success, then repeatedly executes Application and idle. This provides a strong initialization boundary: no normal IRQ should observe partially initialized service/Application state.

For this example, the exact resource behavior is summarized in its [README](../README.md). The important architectural question is the order implied by `system_init.c`: lower-level board/peripheral invariants are established before a service exposes them, and service state exists before Application starts using it.

## Source map and exact composition

### Exact composition

```text
system_init
  -> board_init
       -> RCC HSE/PLL attempt or HSI fallback
       -> board_led_init
       -> board_button_init -> PA0 + EXTI0 + NVIC priority 2
       -> board_timebase_init -> SysTick
  -> time_service_init
  -> indication_service_init
  -> button_service_init
  -> event_service_init
  -> application_init
```

The button path crosses `application.c -> button_service.c -> board_button.c -> mcal_exti.c/mcal_gpio.c`. Time comes through `time_service.c -> board_timebase.c -> mcal_systick.c`. This separates an EXTI edge from the later semantic acceptance of a press.

## Resource ownership

A resource is considered owned by the lowest layer that must know its implementation detail:

| Concern | Owner | Reason |
|---|---|---|
| Application behavior/state | `app` | defines what the demo/product does |
| Logical capability/state | `services` | hides board/peripheral representation |
| Blue Pill pins and peripheral instance | `bsp/bluepill` | board-specific mapping |
| External IC command protocol | `ecual` when present | reusable independently of MCU bus instance |
| STM32 peripheral register sequence | `mcal` | MCU-specific but board-independent behavior |
| addresses/register structs/bit fields | `platform/device` | device register contract |
| PRIMASK/NVIC/SysTick/SCB/core ops | `platform/arch` | Cortex-M3 contract |
| startup wiring/composition/fault policy | `system` + `startup` | program construction/runtime boundary |

The key consequence is that Application can be read without RM0008 open beside it. RM0008 becomes necessary when reading MCAL/platform code, exactly where the register-level knowledge is supposed to live.

## Data and control flow

**Interrupt capture**

```mermaid
flowchart TB
    EDGE["PA0 falling edge"] --> IRQ["EXTI0_IRQHandler"]
    IRQ --> CLEAR["Clear EXTI pending bit"]
    CLEAR --> LATCH["Latch line bit in event mask"]
```

**Thread-mode qualification**

```mermaid
flowchart TB
    APP["application_process()"] --> TAKE["button_service_take_press()"]
    TAKE --> EVENT["Atomically take EXTI event"]
    EVENT --> WAIT["Start / restart 30 ms window"]
    WAIT --> SAMPLE["Later: sample PA0"]
    SAMPLE -->|"still active"| PRESS["Return press = true"]
```

EXTI interrupt context is the sole producer of the latched event bit. Thread mode atomically consumes that bit and owns the debounce deadline and GPIO re-sampling policy.

## Concurrency model

SysTick advances the timebase and EXTI0 records the falling edge. Debounce state remains entirely in thread mode. `mcal_exti_take_event()` saves PRIMASK, disables interrupts for the test-and-clear operation, and restores the prior interrupt state, preventing a lost event during consumption.

## Failure model

GPIO/EXTI/timebase initialization failures propagate to `system_init()` and then `system_panic()`. At runtime, a latched EXTI bit can coalesce repeated edges before thread-mode consumption; this is part of the event-bit contract rather than an error condition. A debounce check that finds PA0 released simply returns no press. Core faults remain fail-stop.

## Invariants

1. EXTI hardware pending must be cleared before returning from the ISR.
2. ISR event capture may coalesce edges but must not lose the fact that at least one pending edge occurred.
3. Thread-mode event take must be atomic relative to ISR OR operations.
4. Debounce acceptance requires both elapsed time and a still-active physical level.
5. Application sees only a semantic press and never reads EXTI/GPIO registers.

These invariants define the current ownership and concurrency contract. Violating one changes the runtime model and requires corresponding implementation and verification changes.

## Why this structure matters

A register-level project can easily become unmaintainable if every layer knows every bit field. This repository instead uses direct register access to make the hardware mechanism visible **and** a dependency boundary to keep that mechanism local.

That yields three practical learning benefits:

1. You can trace a high-level request down to the exact register writes.
2. You can change hardware binding without rewriting Application policy.
3. You can reason about ISR/shared-state ownership because there are few legal places where the same resource may be touched.

The cost is more source files and explicit interfaces than a single-file tutorial. That cost is deliberate: the project is practicing firmware architecture at the same time as register programming.

## References

- [STMicroelectronics — STM32F1 Series Documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32f1-series/documentation.html)
- [STMicroelectronics — RM0008: STM32F101/102/103/105/107 Reference Manual](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STMicroelectronics — STM32F103C8 Product Page](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
- [Arm — Cortex-M3 Devices Generic User Guide](https://developer.arm.com/documentation/dui0552/latest/)
- [GNU Binutils — GNU linker documentation](https://sourceware.org/binutils/docs/ld/)
- [OpenOCD User's Guide](https://openocd.org/doc/html/)

---

[← Example README](../README.md) · [↑ Examples](../../README.md) · [Porting guide →](porting_guide.md)
