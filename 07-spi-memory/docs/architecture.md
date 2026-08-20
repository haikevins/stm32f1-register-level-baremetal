# 07 SPI memory - Architecture

> **Focus:** ownership, dependency direction, initialization ordering, execution contexts, data lifetime, and failure invariants for `07-spi-memory`.

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
       -> board_timebase_init
       -> board_led_init; force LED off
       -> board_memory_bus_init(active PCLK2)
       -> 10 ms busy power-on delay
  -> time_service_init
  -> indication_service_init
  -> memory_service_init
       -> w25q64_init -> JEDEC validation
  -> application_init
       -> erase -> program -> read -> verify
```

The protocol stack is `Application -> memory_service -> w25q64 ECUAL -> board_memory_bus transport -> mcal_spi/mcal_gpio`. This lets the W25Q64 code own NOR semantics (WEL, BUSY, page/sector geometry) without knowing that SPI1 is on PA5/PA6/PA7 or CS is PA4.

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
    INIT["Read JEDEC ID"] --> ID{"Expected device?"}
    ID -->|"no"| FAILINIT["Initialization fails"]
    ID -->|"yes"| ERASE["Erase test sector"]
    ERASE --> PROGRAM["Program 32 bytes"]
    PROGRAM --> READ["Read back"]
    READ --> VERIFY{"Data equal?"}
    VERIFY -->|"yes"| HEART["500 ms heartbeat"]
    VERIFY -->|"no"| SOLID["LED solid ON"]
```

The expected JEDEC identity is manufacturer `0xEF` with capacity ID `0x17`, and the destructive test sector is `0x007FF000`. Runtime test failures increment `application_memory_error_count`.

Application owns the test pattern and readback buffers. SPI transactions are synchronous and single-client; chip-select ownership is contained inside the board transport for the duration of each W25Q64 command.

## Concurrency model

SPI and W25Q64 operations are synchronous thread-mode transactions; there is no SPI interrupt, DMA, or bus arbiter. SysTick is used only for the post-test heartbeat. The bus has one client, so no runtime mutex or critical section is required for chip-select ownership.

## Failure model

A JEDEC-ID initialization failure prevents normal startup and leads to `system_panic()`. The destructive erase/program/read/verify self-test is handled differently: a failed operation increments `application_memory_error_count`, leaves `application_memory_test_passed` false, and thread mode drives PC13 solid ON. A successful test switches to the 500 ms heartbeat.

## Invariants

1. CS must be high when idle and surround exactly one protocol transaction.
2. Writes/erases require WEL confirmation and BUSY completion before success is reported.
3. A Page Program must never cross a 256-byte page boundary.
4. Application's test address must remain inside device geometry and is destructive by design.
5. External-device commands live in ECUAL; SPI CR1/SR/DR logic remains in MCAL.
6. A device-init failure and an operational self-test failure intentionally have different system outcomes.

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
