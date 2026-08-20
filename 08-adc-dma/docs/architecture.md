# 08 ADC DMA - Architecture

> **Focus:** ownership, dependency direction, initialization ordering, execution contexts, data lifetime, and failure invariants for `08-adc-dma`.

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

```mermaid
flowchart TD
    APP["app: demo/product policy"] --> SVC["services: logical capability"]
    SVC --> BSP["bsp/bluepill: physical resource binding"]
    SVC --> ECUAL["ecual: external component protocol when used"]
    BSP --> MCAL["mcal: generic STM32 peripheral behavior"]
    ECUAL --> MCAL
    MCAL --> DEV["platform/device: memory map + register fields"]
    MCAL --> ARCH["platform/arch: Cortex-M3 core operations"]
    SYS["system: composition root"] -. constructs .-> BSP
    SYS -. constructs .-> SVC
    SYS -. constructs .-> APP
```

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
       -> board_led_init; force LED off
       -> board_adc_dma_init(PCLK2, APB1 timer clock)
            -> PA0 analog
            -> DMA1 CH1 circular buffer
            -> TIM3 TRGO generator
            -> ADC1 channel 0 + calibration
            -> NVIC DMA1_CH1 priority 1
            -> start DMA, then TIM3
  -> adc_service_init
  -> indication_service_init
  -> application_init
```

The hardware start order matters: buffer and conversion path are configured before the trigger timer is started. Once global IRQs are enabled, DMA HT/TC events can publish blocks. Thread mode then follows `application_process -> adc_service_process -> board_adc_dma_take_sample_block` before applying LED hysteresis.

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
flowchart LR
    TIM["TIM3 update @ 1 kHz"] --> TRGO["TRGO"]
    TRGO --> ADC["ADC1 channel 0"]
    ADC --> DMA["DMA1 CH1 circular 64 samples"]
    DMA --> HT["HT: copy samples 0..31"]
    DMA --> TC["TC: copy samples 32..63"]
    HT --> PUB["stable 32-sample published block"]
    TC --> PUB
    PUB --> SVC["ADC service: min/max/avg/mV"]
    SVC --> APP["Application: 1800/1500 mV LED hysteresis"]
```

The diagram emphasizes behavior, but data ownership is equally important. State used only by one execution context stays private to that module. Shared ISR/thread state is either divided by producer/consumer ownership or protected with a short PRIMASK critical section. External-device payload buffers remain statically allocated and have an explicit owner during synchronous transactions.

## Concurrency model

This example has three execution agents: TIM3 generates trigger events in hardware, DMA writes memory autonomously, and the CPU alternates between DMA ISR and thread mode. Correctness depends on ownership of *which half is stable*. Copying a completed half in ISR prevents DMA from later rewriting the source while the service analyzes it. The design pays ISR-copy cost to avoid a more complex zero-copy buffer-state protocol.

The firmware is a single-core Cortex-M3 super-loop with interrupt preemption. There are no RTOS tasks, so “thread mode” here means the code running from `main()` outside exception handlers. Concurrency therefore comes from hardware peripherals, DMA, and interrupt exceptions rather than parallel CPU threads.

Critical sections save the existing PRIMASK state and restore it, rather than blindly enabling interrupts on exit. That matters because a helper can be called from a context where interrupts were already disabled.

## Failure model

The dominant initialization failure contract is boolean/status propagation upward:

```text
MCAL/BSP/ECUAL initialization failure
              |
              v
       system_init() == false
              |
              v
        system_panic()
```

Runtime failures that the example intentionally tolerates are represented in module/Application state rather than necessarily panicking. The README documents the exact distinction for this example. No exception handler attempts dynamic recovery; core faults converge on panic for deterministic debug behavior.

Timeouts are bounded loops rather than scheduler-based deadlines in lower-level startup-sensitive paths. This keeps initialization independent of interrupts, but a “poll count” should not be mistaken for a portable wall-clock duration.

## Invariants

1. DMA writes only the circular acquisition buffer; service never analyzes that live memory directly.
2. ISR publishes only a half after DMA signals HT or TC completion.
3. `g_block_ready` defines single-slot ownership; publishing over an unconsumed block increments overrun before replacement.
4. Thread copy/ready-clear is atomic relative to DMA ISR publication.
5. ADC clock must remain within configured/hardware limit and trigger rate must be exactly derived from the active timer clock.
6. Application consumes processed measurements, never ADC/DMA registers or raw DMA pointers.

These invariants are more useful than memorizing call order. If a future change violates one, the design has changed and documentation/tests should be updated intentionally.

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
