# Architecture — 03-uart-polling

## 1. Dependency Graph

```text
Application
    |
UART Service
    |
Board UART
    |
MCAL USART + GPIO
    |
Platform Device
```

## 2. Responsibilities

Application owns greeting and echo policy.

Service exposes byte operations.

BSP owns PA9/PA10 and USART1 mapping.

MCAL owns USART/GPIO registers and baud calculation.

Platform Device owns base addresses and bit definitions.

## 3. Public Contract

The upper-layer contract is non-blocking:

```c
bool try_read(...);
bool try_write(...);
```

`false` means the operation did not complete now.

## 4. Polling Ownership

Only MCAL inspects USART status bits such as RXNE/TXE.

Application never polls a register.

## 5. Error Handoff

This baseline example intentionally keeps detailed UART errors out of the
public API.

A richer Service could expose error counters without moving register bits
upward.

## 6. No Interrupt Concurrency

USART1 is entirely thread-driven.

There is no ISR/thread race for UART state in this example.

That makes it the clean baseline before Example 04.

## 7. Timing Dependency

BRR depends on the actual APB2 clock.

The RCC/Board layer provides the real peripheral clock, including HSI fallback.

## 8. Initialization Dependency

```text
RCC clock
    |
GPIO + USART1
    |
Service
    |
Application
```

Application cannot send the greeting before UART initialization is complete.

## 9. Layer Boundary

Application knows:

```text
byte input
byte output
```

It does not know:

```text
PA9
PA10
USART1
SR
DR
BRR
PCLK2
```

## 10. Failure Path

Invalid board/MCAL initialization returns failure upward where supported.

Runtime "not ready" is not a fatal error; it is represented by a `false`
non-blocking result.
