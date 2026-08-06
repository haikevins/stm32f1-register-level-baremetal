# 03-uart-polling

Register-level bare-metal USART1 polling example for the STM32F103C8T6
Blue Pill.

The firmware configures USART1 for 115200 baud, 8 data bits, no parity and
one stop bit. It sends a short greeting and then echoes every received byte.
All UART functions are non-blocking: the super-loop checks `RXNE` and `TXE`
and immediately returns when the peripheral is not ready.

## Wiring

Use a 3.3 V USB-to-UART adapter:

```text
Blue Pill PA9  USART1_TX  -> adapter RX
Blue Pill PA10 USART1_RX  <- adapter TX
Blue Pill GND             -> adapter GND
```

Do not connect a 5 V UART signal directly to PA10.

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
STM32F103 UART polling ready
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
MCAL USART + GPIO + RCC
    |
    v
USART1 / GPIOA / RCC registers
```

Application code does not include BSP, MCAL or device-register headers.

## Polling API

```c
bool serial_service_try_read_byte(uint8_t *byte);
bool serial_service_try_write_byte(uint8_t byte);
```

These functions never wait. A `false` result means no byte is currently
available or the transmitter data register is not ready.

This example intentionally does not implement `USART1_IRQHandler`. The
startup vector therefore keeps the weak default USART1 handler.

## Debug variables

The following symbols are useful in GDB:

```gdb
p/x application_uart_last_rx_byte
p application_uart_rx_count
p application_uart_tx_count
p application_uart_error_events
p/x application_uart_error_flags
```

Error flag values:

```text
bit 0: parity error
bit 1: framing error
bit 2: noise error
bit 3: overrun error
```

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

## Debug

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

## Idle behavior

`system_idle()` executes `NOP` instead of `WFI`. Polling requires the CPU to
continue checking USART status flags, and this also keeps SWD attachment
reliable with an ST-Link setup that does not expose NRST.
