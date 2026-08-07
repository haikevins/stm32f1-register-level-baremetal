# Architecture

## Dependency direction

```text
app -> services -> bsp -> mcal -> platform
```

`system` is the composition root. It initializes the board and services, then
calls the application super-loop.

## Responsibilities

### Application

- Queues the startup greeting.
- Dequeues received bytes.
- Retains one pending echo byte when the TX ring is full.
- Exposes counters for debugger inspection.

### Serial service

- Presents a board-independent byte-oriented non-blocking API.
- Reports accumulated receive errors and software RX overflow counts.

### BSP

- Maps the console UART to USART1.
- Maps TX to PA9 and RX to PA10.
- Supplies the actual APB2 clock and configured baud rate to MCAL.

### MCAL

- Configures GPIO and USART1 through direct register access.
- Owns static RX and TX ring buffers.
- Handles RXNE and TXE interrupts.
- Records hardware receive errors and software RX-ring overflow.
- Configures the USART1 NVIC line.

### Platform

- Defines STM32F103 memory addresses, register structures, IRQ numbers and bit
  masks.

## Interrupt policy

`USART1_IRQHandler` is a strong MCAL symbol. It only transfers bytes and
records error state. Echo behavior remains in thread mode.
