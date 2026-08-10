# STM32F103 Register-Level Bare-Metal Template

## 1. When to Use This Template

Use this template when starting a small STM32F103C8T6 project that should:

- access peripherals through your own MCAL and register definitions;
- own startup and linker behavior;
- keep Application independent from hardware registers;
- use explicit initialization order;
- use a cooperative super-loop;
- avoid HAL/LL/SPL/RTOS framework ownership.

The template is intentionally minimal. It is a foundation for building new
examples or projects, not a finished application.

## 2. Current Target

| Item | Value |
|---|---|
| MCU | STM32F103C8T6 |
| CPU | Arm Cortex-M3 |
| Flash | 64 KiB |
| SRAM | 20 KiB |
| External crystal | 8 MHz HSE |
| Normal clock target | 72 MHz |
| Language | C11 + GNU assembler |
| Build | GNU Make |
| Debug | OpenOCD + GDB |
| Peripheral access | custom register definitions |

## 3. Directory Structure

```text
app/
services/
ecual/
bsp/bluepill/
mcal/
platform/
common/
config/
system/
startup/
linker/
tests/
tools/
docs/
```

The directory layout represents dependency ownership.

## 4. Target Architecture

```text
Application
    |
    v
Services
    |
    +------> BSP
    |
    +------> ECUAL
                  |
                  v
                 MCAL
                  |
                  v
          Platform Device
                  |
                  v
       Platform Architecture
```

`system/` is the composition root.

## 5. Dependency Rules

### Application

May depend on:

```text
Application
Services
Common
Config
```

Must not include BSP, ECUAL, MCAL, Platform, or register headers.

### Services

May depend on:

```text
Services
BSP
ECUAL
Common
Config
```

Must remain independent from raw STM32 registers.

### BSP

Owns physical board resources:

- pins;
- peripheral instance selection;
- active polarity;
- board wiring;
- composition of MCAL resources.

BSP may depend on MCAL.

### ECUAL

Owns off-chip device protocols such as:

- SSD1306;
- W25Q64;
- sensors;
- EEPROMs.

ECUAL should depend on generic transport APIs rather than a particular board
pin map.

### MCAL

Owns MCU peripheral behavior:

- RCC;
- GPIO;
- SysTick;
- EXTI;
- NVIC;
- USART;
- timers;
- I2C;
- SPI;
- ADC;
- DMA.

MCAL depends on Platform Device and Platform Architecture where required.

### Platform

Platform Device owns:

- base addresses;
- register structures;
- bit definitions;
- IRQ numbers.

Platform Architecture owns Cortex-M3 core instructions and registers.

## 6. Layer Checker

Run:

```bash
make check-layers
```

The checker rejects forbidden project-header dependencies.

It is intentionally simple and should be treated as a guardrail, not a
replacement for design review.

## 7. Startup Sequence

The startup assembly provides the vector table and Reset Handler.

Conceptual sequence:

```text
Reset_Handler
    |
    +--> initialize stack from vector table
    +--> copy .data
    +--> zero .bss
    +--> call main()
```

Unused handlers are weak aliases to `Default_Handler`.

A module takes ownership of an interrupt by implementing the exact strong
handler name expected by the vector table.

## 8. Runtime Initialization

The common runtime pattern is:

```text
main()
    |
    +--> disable global IRQ
    +--> system_init()
    +--> enable global IRQ
    +--> super-loop
```

This means initialization code cannot assume an interrupt-driven timebase is
already advancing.

If startup hardware settling is required before IRQ enable, use a bounded
busy-wait implementation designed for that phase.

## 9. Linker Script

The linker script defines:

- FLASH origin/length;
- RAM origin/length;
- vector placement;
- `.text`;
- `.data`;
- `.bss`;
- stack top symbols.

When porting to another MCU density, update the linker memory geometry before
trusting the build.

## 10. Vector Table and Interrupt Extension

To add an interrupt:

1. verify the vector name in startup;
2. configure the peripheral;
3. clear stale pending flags;
4. configure the NVIC through MCAL;
5. implement the strong handler in the lowest owning module;
6. keep the ISR bounded;
7. hand data/events to thread mode.

## 11. Fault Handling

Fault handlers default to the startup `Default_Handler` unless a project
provides dedicated handlers.

A production project can add:

- HardFault diagnostics;
- stacked-register capture;
- reset reason logging;
- watchdog recovery.

Keep those mechanisms below Application policy where possible.

## 12. `main()` and Composition

The normal `main()` structure is:

```c
int main(void)
{
    cortex_m3_disable_irq();

    if (!system_init())
    {
        system_panic();
    }

    cortex_m3_enable_irq();

    for (;;)
    {
        application_process();
        system_idle();
    }
}
```

Application behavior stays in `app/`.

## 13. Template Idle Behavior

### Using `WFI`

`WFI` is appropriate when:

- the system has reliable interrupt wake sources;
- low-power idle is desired;
- debug/reset behavior is acceptable.

### Using `NOP`

`NOP` is appropriate when:

- predictable SWD attach is more important than low power;
- the debug probe has no NRST connection;
- the educational project should visibly continue executing.

The completed examples use `NOP`.

## 14. Cortex-M3 Abstraction

The Architecture layer exposes helpers such as:

```text
NOP
WFI
enable IRQ
disable IRQ
PRIMASK access
```

Core-register definitions such as NVIC, SCB, and SysTick belong in the
Cortex-M3 platform layer.

Upper layers should not use inline assembly directly.

## 15. Device Layer Skeleton

Platform Device contains the STM32F103 register-level model.

Typical files define:

```text
memory base addresses
peripheral register structs
register bit masks
IRQ numbers
device constants
```

MCAL consumes these definitions.

Application and Services must not.

## 16. Makefile

The Makefile:

- discovers project C/assembly sources;
- applies Cortex-M3 compiler flags;
- links with the STM32F103C8T6 linker script;
- generates binary/hex/listing/map files;
- supports OpenOCD/GDB;
- runs the architecture checker.

## 17. Build Artifacts

Typical output:

```text
build/firmware.elf
build/firmware.hex
build/firmware.bin
build/firmware.lst
build/firmware.map
```

## 18. Make Targets

```bash
make
make clean
make check-layers
make size
make tree
make flash
make erase
make debug-server
make debug
```

## 19. OpenOCD

The default setup uses ST-Link with SWD and:

```tcl
reset_config none
adapter speed 1000
```

This matches a debug connection without NRST.

## 20. GDB

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

Useful first breakpoints:

```gdb
break main
break system_init
break board_init
```

## 21. Creating a New Project from the Template

Recommended order:

1. copy the template;
2. define board pins/resources;
3. add required Platform Device register definitions;
4. implement MCAL peripheral support;
5. implement BSP mapping;
6. add ECUAL if the project has an external device;
7. add Services;
8. add Application behavior;
9. connect initialization in `system_init()`;
10. add ISR handoff where required;
11. document wiring/test behavior;
12. run layer checker;
13. clean-build;
14. hardware-test.

## 22. Completion Checklist

### Architecture

- [ ] Application has no BSP/MCAL/Platform includes.
- [ ] Services contain no register accesses.
- [ ] BSP owns physical board mapping.
- [ ] ECUAL owns external-device protocol.
- [ ] ISR ownership is explicit.

### Runtime

- [ ] `.data` initializes correctly.
- [ ] `.bss` is zeroed.
- [ ] global IRQ lifecycle is intentional.
- [ ] `system_init()` orders dependencies correctly.
- [ ] `application_process()` is bounded.
- [ ] panic behavior is intentional.

### Peripheral

- [ ] base address/register struct/bit masks are correct.
- [ ] peripheral clock/reset logic is correct.
- [ ] GPIO mode is correct.
- [ ] bus/timer clock math is correct.
- [ ] errors/timeouts/overflow are defined.

### Tooling

- [ ] `make check-layers` passes.
- [ ] clean build passes.
- [ ] map/size are reasonable.
- [ ] OpenOCD connects.
- [ ] GDB symbols are usable.

### Docs

- [ ] README explains wiring and behavior.
- [ ] architecture document explains ownership.
- [ ] porting guide explains clock/pin/IRQ changes.

## 23. Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/adding_a_module.md`](docs/adding_a_module.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)

## 24. License

See `LICENSE`.
