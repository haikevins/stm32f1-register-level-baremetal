# Porting Guide — 08-adc-dma

## 1. Changing ADC Input Pin/Channel

Update both BSP pin mapping and ADC channel number.

Verify the selected STM32 pin actually maps to that ADC channel.

## 2. Changing Sample Rate

Change:

```c
BOARD_ADC_SAMPLE_RATE_HZ
```

Verify timer divisibility and that ADC conversion time can complete before the
next trigger.

## 3. Changing the Timer Trigger

Verify the selected timer/TRGO source is supported by the STM32F103 ADC external
trigger selection.

Update MCAL trigger selection and BSP composition.

## 4. Changing DMA Buffer Size

Requirements:

```text
>= 2
even
fits DMA CNDTR
```

Block size remains half the circular buffer unless the design is changed.

Recalculate block period and SRAM use.

## 5. Changing DMA Channel

ADC1-to-DMA channel mapping is fixed by the MCU.

Do not choose a DMA channel arbitrarily.

## 6. Changing the ADC Clock Limit

The project target limit is 12 MHz, below the STM32F103 absolute maximum used
by the compile-time guard.

If changing this policy, verify datasheet timing and selected prescaler.

## 7. Changing Reference Voltage

Change `BOARD_ADC_REFERENCE_MV` only if the assumption is appropriate.

For accurate measurements, measure/calibrate VDDA instead.

## 8. Multi-Channel ADC

Add sequence ranks and scan support.

Define clearly how interleaved samples map to channels before changing the
Service.

## 9. Changing IRQ Priority

Review all system interrupts.

DMA service latency must remain short enough to prevent published-block
overruns.

## 10. Refactoring ISR Ownership

If DMA becomes a reusable subsystem, the handler may move to a lower generic
owner.

Preserve the rule that the strong handler belongs to the lowest module owning
DMA1 Channel 1.

## 11. Validation with an Oscilloscope/Debug Pin

A spare debug pin can be toggled on block publication to measure cadence.

At 1 kHz sampling and 32-sample blocks:

```text
block cadence ~= 32 ms
```

## 12. Validation Checklist

-  pin/channel mapping correct;
-  ADC clock within limit;
-  calibration completes;
-  TIM3 trigger rate correct;
-  DMA1 CH1 active;
-  HT/TC events alternate;
-  sequence increments;
-  error count zero;
-  overrun zero at normal load;
-  raw values follow voltage;
-  hysteresis works.

## 13. Common Pitfalls

- GPIO not in analog mode;
- wrong ADC channel;
- ADC clock too fast;
- wrong external trigger selection;
- wrong DMA channel;
- circular mode missing;
- statistics inside ISR;
- critical section too long;
- assuming VDDA is exactly 3.300 V.
