# Porting Guide — 04-uart-interrupt-ring-buffer

## 1. Parts That Can Remain Unchanged

When changing only physical UART hardware, keep:

```text
Application
UART Service
ring-buffer behavior
```

Replace BSP/MCAL mapping as required.

## 2. Changing the USART Instance

Review:

- base address;
- RCC clock bit;
- APB clock;
- pins;
- IRQ number;
- vector handler name;
- NVIC mapping.

## 3. Changing Buffer Size

Update MCAL configuration.

Requirements:

- at least 2;
- power of two;
- compatible with index width.

Check SRAM use.

## 4. Changing IRQ Priority

Review all interrupt priorities.

UART priority should be sufficient to avoid overrun but the ISR must remain
short.

## 5. Changing Baud/Data Format

Extend MCAL USART setup for new baud/parity/stop/word length.

Keep those details below Service.

## 6. Changing TX/RX Pins

Update BSP pin mapping and AFIO/remap support if required.

## 7. Porting to DMA UART

Keep upper API if possible.

Replace byte-by-byte ISR movement with DMA ownership.

Define:

- DMA channel;
- RX circular semantics;
- TX completion semantics;
- overflow/backpressure behavior.

## 8. Concurrency Validation

Stress:

- ring wraparound;
- full/empty transitions;
- simultaneous producer/consumer activity;
- debugger halt/resume;
- high-rate RX.

Verify no lost TX-start transition.

## 9. Symbol Validation

Inspect the ELF/map and verify the expected strong handler exists.

If moving away from USART1, the new handler must replace the corresponding weak
startup symbol.

## 10. Common Pitfalls

- wrong IRQ number;
- wrong handler name;
- wrong APB clock;
- non-power-of-two ring size;
- multiple producers on one ring;
- leaving TXE interrupt always enabled;
- blocking inside ISR;
- ignoring overflow diagnostics.
