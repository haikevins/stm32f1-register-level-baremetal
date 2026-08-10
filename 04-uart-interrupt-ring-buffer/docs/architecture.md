# Architecture — 04-uart-interrupt-ring-buffer

## 1. Dependency Graph

```text
Application
    |
UART Service
    |
Board UART
    |
MCAL USART
    |
Platform USART/NVIC registers
```

## 2. State Ownership

RX:

```text
ISR owns producer/head updates
thread owns consumer/tail updates
```

TX:

```text
thread owns producer/head updates
ISR owns consumer/tail updates
```

This explicit ownership is the core concurrency rule.

## 3. SPSC Reasoning

A single-producer/single-consumer ring avoids a full mutex when each index has
one writer.

Shared index reads still require careful ordering/atomicity.

The MCAL design restricts buffer sizes so 16-bit index arithmetic remains
valid.

## 4. RX Flow

```text
USART hardware
    |
ISR
    |
RX ring
    |
MCAL read API
    |
Board/Service
    |
Application
```

## 5. TX Flow

```text
Application
    |
Service/Board
    |
MCAL write API
    |
TX ring
    |
ISR
    |
USART DR
```

## 6. Interrupt Rules

The ISR may:

- read/clear status;
- move bytes;
- update counters;
- enable/disable TX interrupt.

It must not perform echo policy or call Application.

## 7. Error Semantics

Two categories are separate:

```text
hardware receive error
software ring overflow
```

Keeping them separate makes diagnosis more meaningful.

## 8. API Backpressure

A non-blocking write can fail when the TX ring is full.

That failure is backpressure to thread mode.

Application/Service decides whether to retry, retain a pending byte, or drop.

## 9. Buffer Capacity

Buffer storage is statically allocated and compile-time validated.

Power-of-two capacity allows efficient index masking.

Static allocation makes SRAM cost explicit.

## 10. Initialization Safety

Rings and counters must be initialized before USART IRQ can run.

Global IRQ is disabled during system initialization, which prevents an early
handler from observing partially initialized state.

## 11. Clock Boundary

USART BRR remains an MCAL concern.

Board provides actual PCLK2.

Application knows only the requested baud rate.

## 12. Extension Points

Possible extensions:

- protocol parser Service;
- line-oriented input;
- DMA-backed UART;
- larger buffers;
- flow control;
- multiple USART instances.

Preserve the same ownership model.
