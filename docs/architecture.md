# Template Architecture and Dependency Rules

## 1. Runtime Layers

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

System is the composition root and is not a layer that Application consumes.

## 2. Dependency Matrix

The project checker enforces a downward dependency direction similar to:

| Source layer | Allowed project dependencies |
|---|---|
| Application | Application, Services, Common, Config |
| Services | Services, BSP, ECUAL, Common, Config |
| ECUAL | ECUAL, MCAL, Common, Config |
| BSP | BSP, MCAL, Common, Config |
| MCAL | MCAL, Platform, Common, Config |
| Platform | Platform, Common, Config |

`system/` may include all layers because it wires the runtime together.

## 3. Application

Application owns product/demo policy.

### Good

```text
if button pressed -> toggle indicator
if 500 ms elapsed -> update state
if ADC voltage high -> turn indicator on
```

### Bad

```text
write GPIO register
set USART BRR
clear DMA flags
configure RCC bits
```

Those are lower-layer responsibilities.

## 4. Services

Services expose stable logical capabilities:

- time;
- button;
- indication;
- serial;
- PWM;
- display;
- memory;
- ADC measurement.

They may debounce, filter, aggregate, or translate units.

## 5. BSP

BSP owns board-specific resource selection:

- pin number;
- port;
- peripheral instance;
- active polarity;
- wiring;
- board-level transport composition.

BSP calls MCAL rather than Platform registers directly.

## 6. ECUAL

ECUAL owns off-chip device semantics.

### Transport Callback Pattern

A reusable external-device driver should receive a generic transport rather
than include BSP or MCAL.

Example:

```text
Display Service
    |
SSD1306 ECUAL
    ^
    |
transport callbacks
    |
Board Display Bus
    |
MCAL I2C
```

This keeps the device driver portable.

## 7. MCAL

MCAL owns generic STM32 peripheral behavior.

Public MCAL APIs should accept generic arguments rather than board pin names.

Examples:

```text
mcal_gpio_configure()
mcal_spi_transfer()
mcal_i2c_init()
mcal_systick_init()
```

## 8. Platform Device

Platform Device owns STM32F103-specific register knowledge:

- base addresses;
- register structures;
- bit definitions;
- interrupt numbers;
- memory map.

MCAL is the main consumer.

## 9. Platform Architecture

Platform Architecture owns Cortex-M3 core behavior:

- NVIC register model;
- SysTick core register model;
- PRIMASK;
- `NOP`;
- `WFI`;
- IRQ enable/disable.

The device layer should not duplicate architecture-core definitions.

## 10. Common

Common contains portable utilities and types.

Examples:

- byte ring buffer;
- measurement structures;
- compiler helpers;
- generic status types.

Common should not depend on board or MCU registers.

## 11. System as Composition Root

System is allowed to know multiple layers because it performs initialization.

Example:

```text
board_init()
time_service_init()
display_service_init()
application_init()
```

System must not become a second Application module.

## 12. Initialization Order

Initialize from lower dependency to upper dependency.

```text
clock/peripheral
    |
board resource
    |
service / external device
    |
application
```

Never initialize a Service before the hardware resource it requires.

## 13. Interrupt Ownership

The lowest module that owns the peripheral owns the strong handler.

Examples:

```text
MCAL SysTick -> SysTick_Handler
MCAL UART    -> USART1_IRQHandler
Board ADC/DMA -> DMA1_Channel1_IRQHandler
```

The handler may publish low-level state upward only through static flags,
counters, buffers, or blocks.

## 14. Interrupt-to-Thread Handoff Patterns

### Event Bit

```text
ISR: event_pending = true
thread: take-and-clear
```

### Counter

Use when every event count matters.

### Ring Buffer

```text
ISR producer -> ring -> thread consumer
thread producer -> ring -> ISR consumer
```

### Block-Ready

```text
DMA IRQ -> completed block -> Service
```

Useful for sampled data.

## 15. Critical Sections

Use a short critical section only around atomic shared-state operations.

Pattern:

```text
save PRIMASK
disable IRQ
copy/read-clear shared state
restore PRIMASK
```

Never keep interrupts disabled while performing:

- I2C polling;
- SPI flash operations;
- formatting;
- long memory copies unless strictly justified.

## 16. Volatile

Use `volatile` when state may change asynchronously.

`volatile` does not guarantee:

- atomic read-modify-write;
- queue correctness;
- mutual exclusion;
- memory ownership.

Use explicit concurrency design.

## 17. Polling API Naming

Prefer:

```text
try_read
try_write
take_event
is_ready
process
```

If an operation may wait, document the timeout or poll bound.

## 18. Error Handling

Represent low-level failure using:

- `bool`;
- error/status enum;
- counter;
- pending event.

Examples:

```text
UART overflow
I2C timeout
SPI timeout
DMA transfer error
ADC calibration failure
```

Do not silently discard important fault information.

## 19. Clock Ownership

Application expresses behavior in:

```text
milliseconds
Hz
baud
permille
millivolts
```

MCAL/BSP converts those values using the actual clock tree.

Application must not know PSC, ARR, BRR, CCR, or ADCPRE values.

## 20. Board Active Level

Active-low hardware must be hidden below the logical Service boundary.

For the Blue Pill LED:

```text
logical ON -> BSP drives PC13 LOW
```

Application still requests "ON", not "LOW".

## 21. External-Device Geometry

External-device geometry belongs to ECUAL/configuration.

Examples:

```text
SSD1306: 128 x 64
W25Q64: 8 MiB, 256-byte pages, 4 KiB sectors
```

Application should not construct raw bus frames.

## 22. Build-Time Configuration

Use `config/` for:

- baud rate;
- timeout;
- buffer size;
- debounce interval;
- bus speed;
- PWM rate;
- sample rate;
- threshold;
- external-device address.

Compile-time validation should reject impossible values.

## 23. Dependency Checker Limitations

The checker validates includes, not behavior.

It cannot detect:

- races;
- incorrect register bits;
- ISR latency;
- bad clock math;
- electrical problems;
- hidden coupling through globals.

Architecture review and hardware testing are still required.

## 24. Architectural Acceptance Checklist

- [ ] Application has no low-level includes.
- [ ] Services contain no register access.
- [ ] BSP owns board mapping.
- [ ] ECUAL owns external-device protocol.
- [ ] MCAL owns generic peripheral behavior.
- [ ] Platform owns register/core definitions.
- [ ] ISR belongs to the lowest owner.
- [ ] ISR work is bounded.
- [ ] thread handoff is explicit.
- [ ] shared state ownership is clear.
- [ ] clocks are translated below Application.
- [ ] errors are observable.
- [ ] `make check-layers` passes.
