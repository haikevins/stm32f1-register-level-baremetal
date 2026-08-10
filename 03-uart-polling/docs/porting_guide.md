# Porting Guide — 03-uart-polling

## 1. Changing Pins While Keeping USART1

On STM32F1, alternate-function remapping may be required for non-default pins.

Update BSP pin mapping and AFIO support if necessary.

Keep Application/Service unchanged.

## 2. Moving to USART2/USART3

Update:

- peripheral instance;
- APB bus clock;
- TX/RX pins;
- base address/register model if not already present;
- RCC enable bit.

USART2/3 are on APB1 rather than APB2.

## 3. Changing Baud Rate

Change `BOARD_UART_BAUD_RATE`.

MCAL should calculate BRR from the active peripheral clock.

Verify the actual baud with a terminal or logic analyzer.

## 4. Changing the Clock Tree

Re-check:

- system clock;
- APB prescaler;
- selected USART peripheral clock;
- BRR formula.

Do not keep a BRR value calculated for 72 MHz.

## 5. Changing Data Format

Extend MCAL configuration for:

- parity;
- word length;
- stop bits.

Keep those details below Application.

## 6. Adding a Blocking API with Timeout

If a blocking helper is required, give it an explicit finite timeout.

Keep the non-blocking API available for super-loop use.

## 7. Porting to Another MCU Family

Preserve:

```text
Application -> UART Service -> BSP
```

Replace MCAL/Platform Device and board pin mapping.

## 8. Post-Port Test

- [ ] TX baud correct;
- [ ] greeting readable;
- [ ] RX works;
- [ ] echo works;
- [ ] HSE and fallback clock both calculate a valid baud if fallback is kept;
- [ ] no unbounded wait;
- [ ] layer checker passes.

## 9. Common Pitfalls

- TX connected to TX;
- no common ground;
- wrong APB clock;
- wrong BRR formula;
- wrong alternate-function mapping;
- using 5 V logic;
- blocking forever on TXE/RXNE.
