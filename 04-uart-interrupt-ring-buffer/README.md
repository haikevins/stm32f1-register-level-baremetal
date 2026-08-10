# 04-uart-interrupt-ring-buffer — USART1 Interrupt + RX/TX Ring Buffer

## 1. Learning Objectives

This example moves UART byte transfer into an interrupt-driven MCAL while
keeping Application non-blocking.

You will learn:

- USART1 RXNE/TXE interrupts;
- static RX/TX ring buffers;
- single-producer/single-consumer ownership;
- TXE interrupt start/stop;
- short critical sections;
- RX overflow and hardware error counters;
- preserving a similar Service API to the polling example.

## 2. Wiring

Same as Example 03:

```text
PA9  USART1_TX  ---> USB-UART RX
PA10 USART1_RX  <--- USB-UART TX
GND              --- common ground
```

Terminal:

```text
115200 8N1
```

## 3. Compile-Time Configuration

```c
#define BOARD_UART_BAUD_RATE     (115200UL)

#define MCAL_USART_RX_BUFFER_SIZE (128UL)
#define MCAL_USART_TX_BUFFER_SIZE (128UL)
#define MCAL_USART1_IRQ_PRIORITY  (2UL)
```

Both buffer sizes must be powers of two and at least two.

## 4. Ring-Buffer Model

The MCAL owns static rings.

### RX Ring

```text
producer: USART1 ISR
consumer: thread mode
```

### TX Ring

```text
producer: thread mode
consumer: USART1 ISR
```

This is a classic single-producer/single-consumer design.

## 5. Empty and Full

The implementation uses head/tail indices and power-of-two masking.

Usable capacity is one element less than the storage size when the design uses
head/tail equality for empty/full distinction.

The exact implementation is documented in MCAL source.

## 6. Initialization Flow

```text
board_init()
    |
    +--> RCC clock/fallback
    +--> board_uart_init(PCLK2)
            |
            +--> PA9/PA10
            +--> MCAL USART1 init
                    |
                    +--> BRR
                    +--> RX/TX rings
                    +--> NVIC
                    +--> RXNE/error IRQ enable
```

TXE interrupt remains disabled until transmit data is queued.

## 7. RX Interrupt Path

```text
RXNE/error interrupt
    |
USART1_IRQHandler
    |
read SR/DR
    |
hardware error?
    |
update counter
    |
push byte to RX ring
    |
ring full?
    |
increment overflow counter
```

Thread mode later pops RX bytes.

## 8. TX Interrupt Path

Thread mode:

```text
queue byte to TX ring
    |
enable TXE interrupt
```

ISR:

```text
TXE active?
    |
pop byte
    |
    +--> byte -> write DR
    +--> empty -> disable TXE interrupt
```

Disabling TXE interrupt when empty prevents repeated useless interrupts.

## 9. Thread-Mode Write and TX-Start Race

The enqueue and TXE-enable sequence must not lose the transition from "no TX
work" to "TX work pending."

The MCAL uses a short PRIMASK-protected critical section around the
enqueue/interrupt-enable operation.

It does not keep interrupts disabled during actual UART transmission.

## 10. Thread-Mode Read

Thread mode reads only from the RX ring.

The ISR writes only to the RX ring.

This producer/consumer ownership avoids a general lock around every byte.

## 11. RX Overflow Semantics

If the RX ring is full when a new byte arrives:

```text
new byte cannot be stored
overflow counter increments
```

The overflow is observable through debug/state APIs.

No heap expansion is attempted.

## 12. Hardware Error Flags

MCAL tracks receive errors such as:

```text
PE
FE
NE
ORE
```

These are hardware-level errors and are distinct from software ring overflow.

## 13. Application Behavior

The Application sends:

```text
STM32F103 UART interrupt ring buffer ready
```

and then echoes bytes.

Because the UART rings absorb asynchronous byte movement, Application does not
need to poll hardware flags.

## 14. Debug Symbols

Useful state includes MCAL counters and ring indices.

Look for:

```text
RX overflow count
RX error count
RX head/tail
TX head/tail
Application echo pending state
```

Use GDB with symbols from the ELF.

## 15. Interrupt Ownership

`USART1_IRQHandler()` belongs to MCAL USART.

It performs:

- flag/error handling;
- RX ring push;
- TX ring pop;
- TXEIE enable/disable support.

It does not call Service/Application.

## 16. Architecture

```text
Application
    |
UART Service
    |
Board UART
    |
MCAL USART
    |
RX/TX rings + NVIC + USART registers
```

## 17. Comparison with Example 03

Example 03:

```text
thread -> poll RXNE/TXE
```

Example 04:

```text
ISR <-> rings <-> thread
```

The upper-level UART concept stays non-blocking.

## 18. Idle Behavior

`system_idle()` uses `NOP`.

UART interrupts can preempt thread mode whenever RX/TX hardware requires
service.

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

## 19. Stress Test

1. Verify greeting.
2. Type single characters.
3. Paste a long burst.
4. Inspect RX overflow counter.
5. Halt the CPU briefly while the host continues sending.
6. Resume and observe diagnostics.
7. Verify TXE interrupt disables when TX ring becomes empty.

## 20. Troubleshooting

### No Greeting

Check TX ring enqueue, TXE interrupt enable, NVIC, handler ownership, and PA9.

### Greeting Stops After the First Byte

Check TXE interrupt lifecycle and whether the handler writes subsequent bytes.

### RX Overflow Increases

The consumer is not keeping up with the producer.

Increase buffer size or reduce/shape input rate if required.

### Hardware Overrun Increases

Check baud mismatch, long interrupt masking, ISR priority, and host send rate.

## 21. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
