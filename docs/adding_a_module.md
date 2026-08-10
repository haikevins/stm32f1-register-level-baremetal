# Adding a New Module

## 1. Start from the Requirement, Not from a Register

Describe the desired behavior first.

Examples:

```text
"Application needs a debounced button event."
"Application needs a 1 kHz PWM output."
"Application needs to store data in external NOR flash."
```

Then decide which layers are required.

## 2. Decide Which Layer Owns the Module

| Responsibility | Layer |
|---|---|
| product policy | Application |
| logical capability | Service |
| board mapping | BSP |
| external-device protocol | ECUAL |
| MCU peripheral implementation | MCAL |
| STM32 register definitions | Platform Device |
| Cortex-M core support | Platform Architecture |
| generic utility | Common |
| initialization order | System |

## 3. Add the MCAL Peripheral

Define a generic MCAL API before writing board-specific code.

### Public MCAL APIs Should Accept Generic Inputs

Prefer:

```c
bool mcal_timer_init(uint32_t timer_clock_hz,
                     uint32_t target_tick_hz);
```

rather than:

```c
bool timer_for_bluepill_led_init(void);
```

Board meaning belongs in BSP.

## 4. Add the Base Address

If the peripheral is not yet modeled, add its base address in Platform Device.

Use the STM32F103 memory map.

Do not scatter numeric addresses through MCAL source.

## 5. Add the Register Structure

Define the register block layout with the correct:

- order;
- offsets;
- `volatile`;
- read-only/read-write qualifiers.

Check reserved gaps carefully.

## 6. Add Bit Definitions

Add named masks/shifts for every register field used by MCAL.

Avoid unexplained hexadecimal literals in MCAL logic.

## 7. Add RCC Clock/Reset Support

A peripheral normally needs:

- clock enable;
- optional peripheral reset;
- correct bus source.

Keep RCC manipulation inside MCAL.

## 8. Add GPIO Alternate-Function Support

Verify each pin mode:

- output push-pull;
- alternate-function push-pull;
- alternate-function open-drain;
- floating input;
- pull-up/pull-down;
- analog.

Do not assume one peripheral's mode applies to another.

## 9. Design Polling

Polling must be:

- immediate/non-blocking, or
- bounded by a timeout/poll count.

Do not introduce an infinite hardware wait.

## 10. Design Interrupt Handoff

### Event Bit

Useful for a single edge.

### Ring Buffer

Useful for byte streams.

### Block Event

Useful for DMA/sample blocks.

The ISR publishes low-level state and returns.

## 11. NVIC

Add/extend MCAL NVIC support rather than configuring NVIC directly from
Application/BSP when a reusable mapping is appropriate.

Configure:

- IRQ number;
- priority;
- pending clear;
- enable/disable.

## 12. Strong Handler Name

The handler must exactly match startup.

Examples:

```c
void EXTI0_IRQHandler(void);
void USART1_IRQHandler(void);
void DMA1_Channel1_IRQHandler(void);
```

## 13. Add a Board Resource

BSP maps the generic MCAL peripheral to the physical board resource.

Examples:

```text
STATUS_LED -> PC13
DISPLAY_I2C -> I2C1 PB6/PB7
MEMORY_SPI -> SPI1 PA4..PA7
```

## 14. `board_pins.h`

Keep physical pin definitions in BSP.

Application and Services should never contain pin numbers.

## 15. Add an External-Device ECUAL

If the new hardware is off-chip, create an ECUAL driver for:

- command set;
- register map;
- device geometry;
- protocol state.

## 16. Transport Callback for ECUAL

Prefer a transport object/function pointers:

```text
write
transfer
select/deselect
delay
```

This allows the ECUAL driver to remain independent from BSP/MCAL.

## 17. Add a Service

Create a Service when Application should consume a logical capability rather
than a specific device.

Examples:

```text
SSD1306 -> Display Service
W25Q64 -> Memory Service
raw ADC block -> ADC Service
```

## 18. Service Processing Pattern

Keep Service processing bounded.

Typical:

```c
void service_process(void)
{
    if (!work_pending())
    {
        return;
    }

    /* bounded processing */
}
```

## 19. Add Application Behavior

Application should express policy only.

Good:

```text
if measurement >= threshold -> indicator on
```

Bad:

```text
if ADC1->DR > 2000 -> GPIOC->BRR = ...
```

## 20. Update `system_init()`

Initialize from bottom to top.

Typical:

```text
board_init()
service_init()
external_device_init()
application_init()
```

Return `false` if a required component fails.

## 21. Global IRQ Lifecycle

Remember that the standard examples disable global interrupts before
`system_init()`.

Therefore:

- SysTick IRQ does not advance during early initialization;
- interrupt-driven waits cannot be used in that phase;
- busy settling delays used before IRQ enable must be independent from SysTick.

This is especially important for external-device power-on delays.

## 22. Add Configuration

Put tunable values in `config/`.

Examples:

```text
clock
frequency
timeout
buffer size
address
threshold
sample rate
debounce
```

## 23. Compile-Time Validation

Reject invalid relationships early.

Examples:

```text
buffer size < 2
DMA sample count odd
frequency = 0
page write > 256
invalid I2C address
ADC clock limit exceeded
```

## 24. Do Not Use Heap Allocation

Prefer:

```text
static buffers
fixed capacities
explicit ownership
```

This improves predictability and ISR safety.

## 25. Add Debug Observability

Expose useful counters/state such as:

```text
overflow count
error count
sequence
JEDEC ID
verification flags
```

Do not add unnecessary I/O solely for debugging when GDB can inspect the
state.

## 26. Update Documentation

### README

Document:

- wiring;
- configuration;
- initialization;
- runtime behavior;
- expected result;
- troubleshooting.

### `architecture.md`

Document:

- ownership;
- data flow;
- ISR boundary;
- concurrency;
- failure path.

### `porting_guide.md`

Document:

- pins;
- clock;
- peripheral instance;
- IRQ/DMA mapping;
- validation.

## 27. Run the Layer Checker

```bash
make check-layers
```

Fix architecture violations instead of weakening the checker.

## 28. Build Cleanly

```bash
make clean
make
```

## 29. Inspect Map/Symbols

Use:

```bash
make size
```

and inspect the map/listing when verifying:

- memory use;
- handler ownership;
- static buffers;
- section placement.

## 30. Hardware Bring-Up Strategy

Bring up in layers:

1. power/wiring;
2. clock;
3. GPIO;
4. peripheral registers;
5. polling/IRQ/DMA;
6. BSP;
7. Service;
8. Application.

## 31. Module Addition Checklist

- [ ] requirement defined;
- [ ] correct layer chosen;
- [ ] base/register/bit definitions added;
- [ ] MCAL API generic;
- [ ] RCC/GPIO setup correct;
- [ ] polling bounded;
- [ ] ISR ownership correct;
- [ ] NVIC configured through proper layer;
- [ ] BSP mapping added;
- [ ] ECUAL transport clean;
- [ ] Service hardware-independent;
- [ ] Application contains no low-level include;
- [ ] IRQ lifecycle considered;
- [ ] compile-time validation added;
- [ ] debug observability added;
- [ ] docs updated;
- [ ] layer checker passes;
- [ ] clean build passes;
- [ ] hardware test passes.
