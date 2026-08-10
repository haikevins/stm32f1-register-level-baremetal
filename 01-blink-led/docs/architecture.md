# Architecture — 01-blink-led

## 1. Dependency Graph

```text
Application
    |
    +--> Time Service ------> Board Timebase -----> MCAL SysTick
    |
    +--> Indication Service -> Board LED ---------> MCAL GPIO
                                                      |
                                                      v
                                               Platform Device
```

## 2. Layer Responsibilities

Application owns the 500 ms blink policy.

Time Service owns millisecond semantics.

Indication Service owns the logical indicator API.

Board Timebase maps the logical timebase to SysTick.

Board LED maps the logical indicator to PC13 and active-low polarity.

MCAL owns register-level peripheral behavior.

Platform owns the STM32/Cortex-M register model.

## 3. Initialization Dependency

```text
RCC clock
    |
Board LED + Board Timebase
    |
Services
    |
Application
```

Application is initialized only after all required hardware resources are valid.

## 4. Runtime Data Flow

### Time Path

```text
SysTick interrupt
    |
MCAL tick counter
    |
Board Timebase
    |
Time Service
    |
Application periodic check
```

### LED Path

```text
Application
    |
Indication Service
    |
Board LED
    |
MCAL GPIO
    |
PC13
```

## 5. Register Ownership

Only MCAL/Platform code knows:

```text
RCC APB2 enable bits
GPIO CRH fields
GPIO BSRR/BRR
SysTick CTRL/LOAD/VAL
```

Application and Services never access those registers.

## 6. ISR Ownership

`SysTick_Handler()` belongs to MCAL SysTick because MCAL owns the core
timebase peripheral.

The ISR performs one bounded action:

```text
tick_count++
```

## 7. Concurrency

The tick counter is written in ISR context and read in thread mode.

The project uses a 32-bit counter and unsigned subtraction for wraparound-safe
elapsed-time calculations.

No blocking synchronization is required for this simple one-writer/read-only
pattern.

## 8. HSE Fallback

Board initialization tries HSE+PLL first.

If that path fails:

```text
HSI 8 MHz
```

is selected.

The Board Timebase receives the actual system clock, so SysTick remains 1 kHz
under either clock source.

## 9. Why Application Does Not Use a Busy Delay

A 500 ms busy delay would prevent the super-loop from performing other work.

The timestamp design instead performs:

```text
check -> not due -> return
```

This is the scheduling model reused by later examples.

## 10. Current Event Service

No general event queue is needed in Example 01.

Adding an event framework would increase complexity without solving a current
requirement.

## 11. Failure Path

If board initialization fails:

```text
board_init() -> false
system_init() -> false
system_panic()
```

The Application never runs with an invalid timebase.

## 12. Extension Boundaries

Good extension points:

- blink policy -> Application;
- logical indicator behavior -> Service;
- pin/polarity -> BSP;
- GPIO/SysTick implementation -> MCAL;
- register definitions -> Platform.

Keep those boundaries intact.
