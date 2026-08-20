# 07 SPI memory - Porting Guide

> **Purpose:** identify which assumptions in `07-spi-memory` are board-specific, STM32F103-specific, Cortex-M3-specific, or application policy before changing hardware.

[← Architecture](architecture.md) · [Example README](../README.md) · [Examples index](../../README.md) · [Root](../../../README.md)

## Table of contents

- [Porting model](#porting-model)
- [Porting checklist](#porting-checklist)
- [Clock and timing verification](#clock-and-timing-verification)
- [Register-level verification](#register-level-verification)
- [Concurrency verification](#concurrency-verification)
- [Validation order](#validation-order)
- [References](#references)

## Porting model

Do not start a port by editing random numeric register constants until the code builds. Classify the change first:

```text
Application policy change
        |
Board-only change --------> BSP / config
        |
Same STM32F103 peripheral -> mostly BSP/config, verify MCAL assumptions
        |
Different STM32F1 part ---> device map/IRQ/pins/Flash-RAM geometry + MCAL review
        |
Different MCU family -----> platform/device + MCAL register sequences + startup/linker
        |
Different CPU arch -------> platform/arch + startup/exception model + atomicity review
```

A clean port preserves the dependency direction. If Application begins including a device header “just for one pin,” the port has bypassed the architectural boundary rather than completed it.

## Porting checklist

| Porting concern | What must be changed or re-verified |
|---|---|
| SPI instance/pins | RCC, AF pin mapping, base address, CS GPIO |
| mode/clock | CPOL/CPHA and maximum clock across voltage/board wiring |
| flash part | JEDEC IDs, density, address width, page/sector geometry, command set |
| supply/interface | 3.3 V requirements and breakout-board circuitry |
| timeout policy | convert poll bounds to an appropriate target-specific timing policy |
| data safety | choose a non-destructive test area and add power-fail strategy |
| shared bus | introduce ownership/arbitration if another SPI client is added |

Also re-run `python3 tools/scripts/check_layers.py` after structural changes.

## Clock and timing verification

The repository attempts 8 MHz HSE -> 72 MHz PLL and falls back to 8 MHz HSI. A port must decide whether that policy is still valid. Verify:

1. oscillator source/frequency and legal PLL multiplication;
2. Flash latency/prefetch requirements at the target clock;
3. APB1 maximum and prescaler;
4. which bus supplies the peripheral;
5. the STM32F1 timer x2 rule when an APB prescaler is not 1;
6. integer/divider limits used by the specific MCAL;
7. timeout-loop meaning at the new CPU clock.

Never preserve a peripheral divider merely because the old board also “ran at 72 MHz.” Derive the clock at the peripheral input and compare it against the target reference manual.

## Register-level verification

The minimal device model is part of the port. For every newly required register:

1. find the peripheral base address and register offset in the reference manual/datasheet;
2. add or extend the `volatile` register structure without disturbing existing offsets;
3. mark read-only fields `volatile const` where the implementation treats them as read-only;
4. define named masks/encodings in `stm32f103xb_register_bits.h` rather than using unexplained literals in MCAL;
5. verify reset state and flag-clear semantics;
6. verify RCC enable/reset bits;
7. verify GPIO mode/remap requirements;
8. verify IRQ number and implemented priority bits if interrupts are involved.

When moving to a different MCU family, prefer creating a new device directory rather than mutating `stm32f103xb` until it no longer describes STM32F103.

## Concurrency verification

SPI and W25Q64 operations are synchronous thread-mode transactions; there is no SPI interrupt, DMA, or bus arbiter. SysTick is used only after initialization for the heartbeat. CS ownership is wholly inside the board transport during a transaction. Because the bus is single-client in this example, the design does not yet need a mutual-exclusion policy across multiple SPI devices.

On a port, re-check every assumption about:

- access width and alignment of shared variables;
- interrupt priority/preemption;
- whether an ISR and thread have single-writer ownership;
- whether PRIMASK is still the desired critical-section mechanism;
- whether DMA or peripheral hardware can overwrite memory while thread mode reads it;
- whether a blocking/polling transaction still fits the cooperative-loop latency budget.

A compiler-clean port is not proof of a correct handoff protocol.

## Validation order

Recommended validation sequence:

```text
1. source layer check
2. compile + link + inspect map/size
3. inspect generated disassembly around startup/ISR/critical paths
4. OpenOCD connect + reset/halt
5. verify clocks and GPIO modes in debugger
6. validate the peripheral with a scope/logic analyzer/terminal as appropriate
7. inject error/timeout/full-buffer conditions
8. verify Application-observable counters/state
```

If the target is still STM32F103C8T6 but only wiring changed, most failures should be diagnosable at BSP/config first. If the MCU changes, verify the platform/device model before debugging higher layers.

## References

- [STMicroelectronics — STM32F1 Series Documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32f1-series/documentation.html)
- [STMicroelectronics — RM0008: STM32F101/102/103/105/107 Reference Manual](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STMicroelectronics — STM32F103C8 Product Page](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
- [Arm — Cortex-M3 Devices Generic User Guide](https://developer.arm.com/documentation/dui0552/latest/)
- [GNU Binutils — GNU linker documentation](https://sourceware.org/binutils/docs/ld/)
- [OpenOCD User's Guide](https://openocd.org/doc/html/)

---

[← Architecture](architecture.md) · [↑ Example README](../README.md) · [08 ADC DMA →](../../08-adc-dma/README.md)
