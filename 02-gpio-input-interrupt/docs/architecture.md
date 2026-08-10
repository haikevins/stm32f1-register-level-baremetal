# Architecture — 02-gpio-input-interrupt

## 1. Layer Diagram

```text
Application
    |
    +--> Event Service
    +--> Button Service
    |       |
    |       +--> Board Button -> MCAL GPIO/EXTI/NVIC
    |       +--> Time Service -> Board Timebase -> MCAL SysTick
    |
    +--> Indication Service -> Board LED -> MCAL GPIO
```

## 2. Ownership

| Concern | Owner |
|---|---|
| PA0 pin/polarity | BSP |
| GPIO/EXTI registers | MCAL |
| raw interrupt edge | MCAL/BSP low-level path |
| debounce | Button Service |
| event queue | Event Service |
| LED toggle policy | Application |

## 3. Why Debounce Is Not in the ISR

Mechanical bounce lasts much longer than an acceptable ISR.

Busy-waiting in EXTI0 would:

- block lower-priority interrupts;
- increase latency;
- mix physical capture with policy;
- make timing fragile.

The ISR records an edge and returns.

## 4. EXTI Event Lifecycle

```text
physical falling edge
    |
EXTI pending
    |
ISR clears flag + records edge
    |
Button Service starts debounce
    |
30 ms elapsed
    |
sample PA0
    |
Event Service publishes pressed event
    |
Application consumes event
```

## 5. Concurrency Model

Interrupt context only publishes low-level edge state.

Thread mode performs:

- debounce;
- queue operations;
- Application policy.

Critical sections are used only where an atomic read/clear operation is
required.

## 6. Initialization Order

```text
RCC
    |
GPIO/AFIO/EXTI/NVIC
    |
Timebase
    |
Services
    |
Application
```

Global IRQ is enabled only after successful initialization.

## 7. Layer Boundaries

Application may include Service headers.

It must not include:

```text
mcal_exti.h
mcal_gpio.h
stm32f103xb.h
cortex_m3_registers.h
```

## 8. Generic EXTI MCAL

MCAL EXTI should describe:

```text
line
edge
port mapping
IRQ behavior
```

without knowing that line 0 is a "user button."

That meaning belongs in BSP.

## 9. Failure Propagation

Fatal initialization failure returns upward:

```text
MCAL/BSP failure
    |
board_init() false
    |
system_init() false
    |
system_panic()
```

Mechanical bounce is not a fatal error; it is a normal Service filtering case.

## 10. Extension Strategy

Keep richer input behavior above the raw hardware:

```text
MCAL/BSP raw edge
    |
Button Service
    |
Event Service
    |
Application
```

Add features without pushing product logic downward.
