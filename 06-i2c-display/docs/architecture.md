# 06 I2C display - Architecture

> **Focus:** ownership, dependency direction, initialization ordering, execution contexts, data lifetime, and failure invariants for `06-i2c-display`.

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
       -> board_display_bus_init(active PCLK1)
            -> PB6/PB7 AF open-drain
            -> I2C1 bounded polling
  -> time_service_init
  -> display_service_init
       -> ssd1306_init(transport callbacks)
  -> application_init
       -> render first frame
       -> present full framebuffer
```

The stack is intentionally split at the transport seam: Application calls `display_service`; the service uses `ssd1306.c`; SSD1306 calls board transport callbacks; board transport calls `mcal_i2c.c`. Only the last layer knows STM32 I2C register semantics.

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
flowchart TB
    APP["Application draws UI state"] --> FB["SSD1306<br/>1024-byte framebuffer"]
    FB --> UPDATE["ssd1306_update()"]
    UPDATE --> BSP["Board display transport"]
    BSP --> CMD["I2C1 command write<br/>control 0x00"]
    BSP --> DATA["I2C1 data write<br/>control 0x40"]
```

The command phase programs SSD1306 addressing; the data phase transfers the framebuffer. Each I2C write completes synchronously with either success or a bounded failure result.

The SSD1306 ECUAL owns the 1024-byte framebuffer. Application mutates it only through display-service calls, and the synchronous update path reads it entirely from thread mode.

## Concurrency model

SysTick advances the application schedule after initialization. I2C transfers are synchronous bounded-polling transactions in thread mode, and the SSD1306 framebuffer is never touched from an ISR. Because global interrupts are still disabled during early startup, the display power-on delay uses a CPU busy-loop rather than the SysTick timebase.

## Failure model

Display-bus or SSD1306 initialization failure prevents normal startup and leads to `system_panic()`. After startup, a failed `display_service_present()` increments `application_display_error_count`, clears the operational flag, and stops further display updates without panicking. MCAL I2C waits are bounded so a stuck bus cannot trap the CPU in an unbounded polling loop.

## Invariants

1. SSD1306 protocol code has no dependency on STM32 I2C registers.
2. BSP owns address, I2C instance, pins, and control-prefix transport.
3. Every hardware wait in MCAL is bounded.
4. Initialization delay cannot require SysTick because global IRQs are disabled.
5. The framebuffer remains stable in ECUAL memory while a synchronous transfer reads it.
6. Application reacts to a boolean presentation result rather than decoding I2C status flags.

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
