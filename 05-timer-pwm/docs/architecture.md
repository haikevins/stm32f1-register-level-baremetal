# 05 Timer PWM - Architecture

> **Focus:** ownership, dependency direction, initialization ordering, execution contexts, data lifetime, and failure invariants for `05-timer-pwm`.

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
       -> board_timebase_init(active SYSCLK)
       -> board_pwm_init(active APB1 timer clock)
            -> PA0 alternate-function output
            -> TIM2 CH1 PWM
  -> time_service_init
  -> pwm_service_init
  -> application_init
```

The control path is `application.c -> pwm_service.c -> board_pwm.c -> mcal_timer.c`. SysTick schedules duty updates, but TIM2 produces the PWM edges independently. That separation is the architectural point of the example: software owns setpoint changes; timer hardware owns waveform timing.

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

```mermaid
stateDiagram-v2
    [*] --> RAMP_UP
    RAMP_UP --> RAMP_DOWN: reach 1000
    RAMP_DOWN --> RAMP_UP: reach 0
```

Every 10 ms, `RAMP_UP` adds 10 permille and `RAMP_DOWN` subtracts 10 permille. The direction changes only at the 0 and 1000 permille limits.

Application owns the ramp direction and logical duty value. MCAL/BSP own timer configuration; once enabled, TIM2 generates PWM autonomously between super-loop iterations.

## Concurrency model

SysTick asynchronously advances the software timebase. PWM generation itself is hardware autonomous: thread mode updates CCR1 while preload is enabled, and TIM2 transfers the new compare value on an update event. No PWM ISR or software critical section is used in the active path.

## Failure model

Clock/GPIO/timer/timebase initialization failures propagate to `system_panic()`. After initialization, the duty ramp has no separate runtime error state; its bounds are enforced by the application and service configuration. Invalid timer-divider combinations are rejected during initialization rather than tolerated during steady-state execution.

## Invariants

1. PWM frequency is derived from the active APB1 timer clock, never a hard-coded 72 MHz assumption.
2. GPIO alternate-function configuration is owned by BSP/MCAL, not Application.
3. Application supplies logical duty only; it never knows PSC/ARR/CCR encodings.
4. Hardware PWM continues between super-loop iterations without CPU edge servicing.
5. Requested timer tick and PWM frequency must satisfy the driver's exact-integer constraints.

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
