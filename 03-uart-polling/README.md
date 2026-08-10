# 03-uart-polling — USART1 Polling with a Non-Blocking API

## 1. Learning Objectives

This example introduces a byte-stream peripheral without interrupts.

You will learn:

- PA9/PA10 USART1 pin configuration;
- APB2 peripheral clocking;
- BRR calculation at register level;
- 115200 8N1 setup;
- RXNE/TXE polling;
- non-blocking `try_read` / `try_write`;
- a small greeting/echo Application state machine;
- why a pending echo byte is required.

## 2. Wiring

```text
Blue Pill PA9  USART1_TX  ---> USB-UART RX
Blue Pill PA10 USART1_RX  <--- USB-UART TX
Blue Pill GND              --- USB-UART GND
```

Use a 3.3 V adapter.

Terminal:

```text
115200 baud
8 data bits
no parity
1 stop bit
no flow control
```

## 3. Expected Behavior

After reset:

```text
STM32F103 UART polling ready
```

Typed bytes are echoed back.

No USART interrupt, DMA, or ring buffer is used.

## 4. Compile-Time Configuration

```c
#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TARGET_CLOCK_HZ   (72000000UL)
#define BOARD_UART_BAUD_RATE     (115200UL)

#define APPLICATION_UART_GREETING \
    "STM32F103 UART polling ready\r\n"
```

`BOARD_UART_BAUD_RATE` must be non-zero.

## 5. Initialization Flow

```text
board_init()
    |
    +--> RCC HSE/PLL or HSI fallback
    +--> board_uart_init(PCLK2)
            |
            +--> PA9 TX configuration
            +--> PA10 RX configuration
            +--> USART1 register setup
            +--> BRR calculation
            +--> enable USART1
```

Then Application initializes its greeting/echo state.

## 6. GPIO Configuration

### TX — PA9

PA9 is configured as an alternate-function push-pull output.

This lets USART1 drive the TX pin.

### RX — PA10

PA10 is configured as an input suitable for USART1 RX.

The USB-UART adapter drives the line.

## 7. Baud-Rate Register

MCAL computes USART1 `BRR` from:

```text
peripheral clock
requested baud
```

For normal operation:

```text
PCLK2 = 72 MHz
baud  = 115200
```

The code performs integer register-level baud calculation rather than exposing
BRR to Application.

Under HSI fallback, the calculation uses the actual lower PCLK2.

## 8. USART Setup

MCAL configures:

```text
word length: 8 data bits
parity: none
stop bits: 1
RX enabled
TX enabled
USART enabled
```

The example intentionally uses a simple 8N1 configuration.

## 9. Polling Receive Path

```text
uart_service_try_read_byte()
    |
Board UART
    |
MCAL USART
    |
RXNE set?
    |
    +--> no  -> false
    +--> yes -> read DR, return byte
```

The call does not wait for a byte.

## 10. Polling Transmit Path

```text
uart_service_try_write_byte(byte)
    |
TXE set?
    |
    +--> no  -> false
    +--> yes -> write DR, true
```

The function returns immediately when the hardware cannot accept another byte.

## 11. Application State Machine

The Application first sends the greeting one byte at a time.

After the greeting:

```text
no pending echo?
    |
    +--> try_read()
            |
            +--> byte available -> save it
                                 -> set echo pending

echo pending?
    |
    +--> try_write(saved byte)
            |
            +--> success -> clear pending
```

This is cooperative and bounded.

## 12. Why `g_echo_pending` Must Be Kept

RX and TX readiness are independent.

A byte can arrive while TXE is not ready.

Without a pending variable, Application would have to either:

- block waiting for TX; or
- discard the received byte.

The one-byte pending state bridges the two non-blocking operations.

## 13. Error Mapping

This example keeps the API minimal and does not expose detailed receive-error
counters.

Its purpose is to establish the polling model.

Example 04 adds explicit interrupt-driven error and overflow diagnostics.

## 14. Debug Symbols

Useful state includes:

```text
g_greeting_index
g_echo_pending
g_echo_byte
```

Exact names can be confirmed in `app/src/application.c`.

Useful breakpoints:

```gdb
break application_process
break mcal_usart_try_read_byte
break mcal_usart_try_write_byte
```

## 15. Interrupt Policy

`USART1_IRQHandler()` is not used.

The startup weak handler remains active.

All UART work happens in thread mode through polling.

## 16. Idle Behavior

`system_idle()` uses `cortex_m3_nop()`.

The super-loop repeatedly checks RX/TX readiness.

## 17. Architecture

```text
Application
    |
Serial/UART Service
    |
Board UART
    |
MCAL USART + MCAL GPIO
    |
Platform Device
```

Application never touches USART registers.

## Build, Flash, and Debug

```bash
make check-layers
make clean
make
make flash
```

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

## 18. Test Procedure

1. Connect USB-UART.
2. Open a 115200 8N1 terminal.
3. Reset the MCU.
4. Confirm greeting.
5. Type single characters.
6. Confirm echo.
7. Paste a short burst and observe the limitations of a polling/no-buffer
   design.

## 19. Troubleshooting

### No Greeting

Check TX/RX crossover, common ground, PA9 mode, USART clock enable, TXE state,
and BRR.

### Greeting Is Garbage

Check terminal settings and actual peripheral clock/BRR calculation.

### Typing Does Not Echo

If the greeting is correct, focus on PA10, RXNE, and the receive path.

### Bytes Are Lost During Fast Input

This is expected when data arrives faster than the polling Application can
service it.

Use Example 04 for buffered interrupt-driven UART.

## 20. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
