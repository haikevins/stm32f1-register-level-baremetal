# 04-uart-interrupt-ring-buffer

Register-level bare-metal USART1 interrupt and ring-buffer example for the
STM32F103C8T6 Blue Pill.

USART1 receives bytes through the RXNE interrupt and transmits queued bytes
through the TXE interrupt. The interrupt handler only moves bytes between the
peripheral data register and statically allocated ring buffers. The
application performs the echo outside interrupt context.

## Wiring

Use a 3.3 V USB-to-UART adapter:

```text
Blue Pill PA9  USART1_TX  -> adapter RX
Blue Pill PA10 USART1_RX  <- adapter TX
Blue Pill GND             -> adapter GND
```

Terminal settings:

```text
115200 baud
8 data bits
no parity
1 stop bit
no flow control
```

After reset:

```text
STM32F103 UART interrupt ring buffer ready
```

Characters typed in the terminal are echoed back.

## Architecture

```text
Application
    |
    v
Serial Service
    |
    v
Board UART
    |
    v
MCAL USART interrupt + ring buffers
    |
    v
USART1 / GPIOA / RCC / NVIC registers
```

Application code does not include BSP, MCAL or device-register headers.

## Ring buffers

Both buffers contain 128 bytes of static storage and intentionally reserve one
slot to distinguish full from empty:

```text
RX storage: 128 bytes, usable capacity: 127 bytes
TX storage: 128 bytes, usable capacity: 127 bytes
```

Configuration is in `config/mcal_config.h`.

Ownership rules:

```text
RX head: ISR writes
RX tail: thread reads

TX head: thread writes
TX tail: ISR reads
```

Enqueuing a TX byte and enabling `TXEIE` occur inside a short critical section
to prevent a lost-transmit-start race.

## Interrupt behavior

`USART1_IRQHandler()` handles:

- `RXNE`: read `USART1_DR` and push the byte into the RX ring.
- receive errors: record PE, FE, NE and ORE flags.
- `TXE`: pop one byte from the TX ring into `USART1_DR`.
- empty TX ring: disable `TXEIE` until another byte is queued.

The ISR does not:

- echo bytes;
- parse commands;
- call Application or Service functions;
- block or wait;
- allocate memory dynamically.

## Non-blocking API

```c
bool serial_service_try_read_byte(uint8_t *byte);
bool serial_service_try_write_byte(uint8_t byte);
```

`try_read_byte()` returns false when the RX ring is empty.
`try_write_byte()` returns false when the TX ring is full.

## Debug variables

```gdb
p/x application_uart_last_rx_byte
p application_uart_rx_count
p application_uart_tx_count
p application_uart_error_events
p/x application_uart_error_flags
p application_uart_rx_overflow_count
```

Error bits:

```text
bit 0: parity error
bit 1: framing error
bit 2: noise error
bit 3: hardware overrun error
```

`application_uart_rx_overflow_count` counts additional bytes discarded because
the software RX ring was full.

## Build

```bash
make check-layers
make clean
make
```

## Flash

```bash
make flash
```

## Debug ISR

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

Inside GDB:

```gdb
delete
break USART1_IRQHandler
continue
```

## Idle behavior

`system_idle()` executes `NOP`, not `WFI`. USART interrupts and ring buffers
still work normally, while SWD attachment remains reliable with an ST-Link
setup that does not expose NRST.
