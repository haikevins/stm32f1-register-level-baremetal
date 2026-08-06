# Architecture

## Dependency direction

```text
app -> services -> bsp -> mcal -> platform
```

`system` is the composition root. It initializes the board and services, then
calls the application super-loop.

## Responsibilities

### Application

- Sends the startup greeting incrementally.
- Reads one byte without blocking.
- Holds at most one pending echo byte.
- Exposes counters for debugger inspection.

### Serial service

- Presents a board-independent byte-oriented polling API.
- Does not know GPIO pins, USART instances or register layouts.

### BSP

- Maps the console UART to USART1.
- Maps TX to PA9 and RX to PA10.
- Supplies the actual APB2 clock and configured baud rate to MCAL.

### MCAL

- Configures GPIO modes through direct register access.
- Configures USART1 BRR and CR registers.
- Polls `RXNE` and `TXE`.
- Clears and records USART receive error flags.

### Platform

- Defines STM32F103 memory addresses, register structures and bit masks.

## Interrupt policy

No USART interrupts are enabled. `USART1_IRQHandler` remains the weak default
handler from the startup file.
