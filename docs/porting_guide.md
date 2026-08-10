# Porting Guide — Template

## 1. Case A — New Project on the Same Blue Pill

Keep:

- startup;
- linker;
- platform architecture;
- STM32F103 device layer.

Change:

- board resources;
- MCAL modules as required;
- Services;
- Application;
- configuration.

## 2. Case B — Different Board, Same STM32F103C8T6

Usually keep:

```text
app/
services/
ecual/
mcal/
platform/
startup/
linker/
```

Review:

```text
bsp/
config/
tools/openocd/
```

## 3. Case C — STM32F103 with Different Memory Density

Update:

- linker FLASH/RAM sizes;
- startup vector assumptions if the device variant differs;
- device constants;
- OpenOCD target expectations.

Do not assume the C8 memory map is correct for another density.

## 4. Case D — Different STM32F1 Part

Review:

- memory map;
- peripheral base addresses;
- register differences;
- available peripherals;
- IRQ numbers;
- alternate-function mapping;
- clock tree;
- startup vectors;
- linker memory.

MCAL may require partial changes.

## 5. Case E — Different MCU Family but Still Cortex-M

Higher layers can often remain.

Replace or heavily adapt:

```text
Platform Device
MCAL
BSP
startup
linker
clock setup
OpenOCD target
```

Platform Architecture may remain partly reusable if the target is still
compatible Cortex-M.

## 6. Case F — Different Architecture

Port:

- architecture intrinsics;
- interrupt model;
- startup;
- linker;
- critical sections;
- toolchain flags;
- device layer.

The conceptual layered design can still be retained.

## 7. BSP Selection Strategy

Keep logical resource names stable.

Example:

```text
STATUS_LED
USER_BUTTON
DISPLAY_BUS
MEMORY_BUS
ADC_INPUT
```

Only BSP maps those resources to physical pins/peripherals.

## 8. Clock Porting

Validate:

- oscillator source;
- startup timeout;
- PLL settings;
- SYSCLK;
- PCLK1/PCLK2;
- timer multiplier rule;
- flash wait states;
- peripheral frequency limits.

Never copy 72 MHz assumptions blindly.

## 9. GPIO Porting

For each pin verify:

- port base;
- clock enable;
- CRL/CRH field;
- MODE/CNF combination;
- pull behavior;
- active polarity;
- AF routing.

## 10. Interrupt Porting

Verify:

- vector index;
- IRQ number;
- handler name;
- NVIC priority field width;
- pending-clear behavior;
- peripheral flag clear sequence.

## 11. DMA Porting

DMA mapping is highly device-specific.

Verify:

- peripheral-to-channel mapping;
- channel register layout;
- transfer width;
- address increment;
- circular mode;
- interrupt flags;
- clear registers.

## 12. External-Device Driver Portability

Keep ECUAL unchanged when:

- the external device is unchanged;
- the transport semantics are unchanged.

Replace only the board bus/MCAL below it.

## 13. Linker Porting

Update:

- FLASH origin/length;
- RAM origin/length;
- vector placement;
- stack top;
- section alignment.

Then inspect the linked map.

## 14. Startup Porting

The startup file must match:

- architecture;
- vector count/order;
- reset semantics;
- section initialization.

Do not reuse an STM32F103 vector table on an unrelated MCU.

## 15. Toolchain Flags

Review:

```text
-mcpu
-mthumb
-float ABI if applicable
linker script
assembler target
```

## 16. OpenOCD

Change the target configuration when the MCU family/device changes.

Use a conservative adapter speed for first bring-up.

## 17. Debug Reset Strategy

Current setup uses:

```tcl
reset_config none
```

because NRST may not be wired.

If the new board/probe supports NRST reliably, hardware reset may be enabled
after validation.

## 18. Validation by Layer

### Startup

- reset reaches `main`;
- `.data` correct;
- `.bss` zero;
- stack valid.

### Clock

- active clock source correct;
- SYSCLK correct;
- APB clocks correct.

### GPIO

- physical electrical mode correct;
- output polarity correct.

### Peripheral

- register configuration correct;
- polling/IRQ/DMA works.

### Service/Application

- logical behavior remains hardware-independent.

## 19. Automated Checks

Run:

```bash
make check-layers
make clean
make
make size
```

## 20. Port Acceptance Checklist

-  linker matches memory;
-  startup matches device;
-  clock tree verified;
-  register map verified;
-  BSP pins verified;
-  MCAL peripheral verified;
-  IRQ numbers/handlers verified;
-  DMA mapping verified if used;
-  OpenOCD connects;
-  layer checker passes;
-  clean build passes;
-  hardware behavior matches the original logical contract.
