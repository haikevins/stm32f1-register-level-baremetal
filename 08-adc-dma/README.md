# 08-adc-dma — TIM3 Trigger + ADC1 + DMA1 Circular Buffer

## 1. Learning Objectives

This example builds a continuous sampled-data pipeline.

You will learn:

- PA0 analog mode;
- ADC1 clock selection and calibration;
- TIM3 TRGO update trigger;
- deterministic 1 kHz sampling;
- DMA1 Channel 1 circular transfers;
- half/full-transfer IRQ events;
- stable 32-sample block publication;
- ISR/thread concurrency;
- average/min/max/mV processing;
- hysteresis in Application.

## 2. Hardware

Use a potentiometer:

```text
3.3 V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

PC13 is the threshold indicator.

Keep PA0 between GND and VDDA.

## 3. Compile-Time Configuration

```c
#define BOARD_ADC_REFERENCE_MV                   (3300UL)
#define BOARD_ADC_MAX_RAW_VALUE                  (4095UL)
#define BOARD_ADC_MAX_CLOCK_HZ                   (12000000UL)
#define BOARD_ADC_SAMPLE_RATE_HZ                 (1000UL)
#define BOARD_ADC_TRIGGER_TIMER_TICK_HZ          (1000000UL)

#define BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT        (64U)
#define BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT         (32U)
#define BOARD_ADC_CALIBRATION_TIMEOUT_ITERATIONS (1000000UL)
#define BOARD_ADC_DMA_IRQ_PRIORITY               (1U)

#define APPLICATION_ADC_LED_ON_THRESHOLD_MV      (1800U)
#define APPLICATION_ADC_LED_OFF_THRESHOLD_MV     (1500U)
```

Compile-time checks validate buffer geometry and ADC clock limits.

## 4. Data-Rate Model

Sample rate:

```text
1000 samples/s
```

Circular buffer:

```text
64 samples
```

Half-buffer:

```text
32 samples
```

One block completes every:

```text
32 ms
```

HT and TC events alternate at roughly 32 ms intervals.

## 5. Timer Trigger

TIM3 provides the ADC sampling clock.

Target timer tick:

```text
1 MHz
```

Trigger rate:

```text
1 kHz
```

At normal clock:

```text
TIM3 clock = 72 MHz
PSC = 71
ARR = 999
```

At HSI fallback:

```text
TIM3 clock = 8 MHz
PSC = 7
ARR = 999
```

TRGO source is the timer update event.

No TIM3 ISR is used.

## 6. ADC Input Configuration

PA0 is configured as analog input.

ADC1 regular sequence contains:

```text
channel 0
one conversion
right-aligned data
```

Sampling is triggered externally by TIM3 TRGO.

## 7. ADC Clock Selection

The MCAL selects an ADC prescaler so:

```text
ADCCLK <= BOARD_ADC_MAX_CLOCK_HZ
```

Configured maximum:

```text
12 MHz
```

At PCLK2 = 72 MHz, `/6` produces 12 MHz.

At 8 MHz fallback, `/2` produces 4 MHz.

## 8. ADC Reset/Power/Calibration Sequence

MCAL:

1. configures ADC clock prescaler;
2. enables ADC1 clock;
3. resets ADC1;
4. clears/configures control/sample/sequence registers;
5. powers ADC;
6. waits a conservative stabilization loop;
7. starts reset calibration;
8. waits for completion with bounded iteration timeout;
9. starts calibration;
10. waits for completion;
11. enables DMA and external trigger.

Calibration failure aborts initialization.

## 9. DMA Mapping

ADC1 data maps to DMA1 Channel 1.

Configuration:

```text
peripheral address: ADC1->DR
memory: uint16_t[64]
peripheral size: 16 bit
memory size: 16 bit
memory increment: yes
circular mode: yes
priority: high
HT interrupt: yes
TC interrupt: yes
TE interrupt: yes
```

## 10. DMA Interrupt Events

MCAL exposes event bits:

```text
HALF_TRANSFER
TRANSFER_COMPLETE
TRANSFER_ERROR
```

The Board ADC/DMA handler consumes/clears the hardware flags.

## 11. IRQ Handler and Block Publishing

`DMA1_Channel1_IRQHandler()`:

```text
take MCAL DMA events
    |
TE -> increment error count
    |
HT -> publish samples 0..31
    |
TC -> publish samples 32..63
```

Publishing copies the completed half into a stable 32-sample block.

## 12. Why the Block Is Copied in the ISR

DMA circular memory will be reused.

If thread mode processed the DMA half directly for too long, DMA could later
overwrite that memory.

The copy provides a simple ownership boundary:

```text
DMA buffer -> ISR copy -> stable published block
```

The cost is a short 32-sample ISR copy.

## 13. Thread-Mode Handoff

`board_adc_dma_take_sample_block()` uses a short IRQ critical section:

```text
save PRIMASK
disable IRQ
check ready
copy stable block
clear ready
restore PRIMASK
```

The critical section protects only shared state.

## 14. ADC Service Processing

For each 32-sample block:

```text
minimum
maximum
sum
rounded average
millivolts
sequence++
```

Voltage estimate:

```text
mV ~= average_raw * 3300 / 4095
```

The 3300 mV reference is an assumption.

## 15. Application Hysteresis

LED logic:

```text
mV >= 1800 -> LED ON
mV <= 1500 -> LED OFF
1500..1800 -> retain previous state
```

The gap prevents rapid flicker near one threshold.

## 16. Debug Globals

```gdb
p application_adc_average_raw
p application_adc_minimum_raw
p application_adc_maximum_raw
p application_adc_millivolts
p application_adc_sequence
p application_adc_dma_overruns
p application_adc_dma_errors
```

`application_adc_sequence` should continuously increase.

## 17. Interrupt/Symbol Expectations

Strong:

```text
DMA1_Channel1_IRQHandler
```

Unused/weak:

```text
ADC1_2_IRQHandler
TIM3_IRQHandler
```

Sampling itself is hardware-triggered and DMA-driven.

## 18. Initialization Flow

```text
board_init()
    |
    +--> RCC normal/fallback clock
    +--> board LED
    +--> board_adc_dma_init()
            |
            +--> PA0 analog
            +--> DMA1 CH1
            +--> TIM3 trigger
            +--> ADC1 clock/config/calibration
            +--> NVIC DMA1 CH1
            +--> start DMA
            +--> start TIM3

system_init()
    |
    +--> adc_service_init()
    +--> indication_service_init()
    +--> application_init()
```

Global IRQ is enabled only after this completes.

## 19. Architecture

```text
TIM3 -> ADC1 -> DMA1
                 |
                 v
          Board ADC/DMA
                 |
                 v
            ADC Service
                 |
                 v
            Application
                 |
                 v
        Indication Service
```

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

## 20. Test Procedure

1. Connect potentiometer.
2. Flash firmware.
3. Inspect `application_adc_sequence`.
4. Verify it increases.
5. Rotate to GND: raw approaches 0.
6. Rotate to 3.3 V: raw approaches 4095.
7. Verify mV tracks input approximately.
8. Cross 1800 mV: LED turns ON.
9. Drop below 1500 mV: LED turns OFF.
10. Confirm DMA errors/overruns remain zero during normal execution.

## 21. Troubleshooting

### Raw Is Always 0

Check PA0 wiring, analog mode, ADC channel, TIM3 trigger, and DMA activity.

### Raw Is Always 4095

Check for PA0 shorted/high, incorrect potentiometer wiring, or input outside the
expected range.

### DMA IRQ Does Not Run

Check DMA1 clock, Channel 1 mapping, HT/TC interrupt enables, NVIC, ADC trigger,
and TIM3 start.

### `dma_overruns` Increases

Thread mode is not consuming published blocks fast enough.

Debugger halts and long blocking work can cause this.

### mV Does Not Match a Multimeter

The firmware assumes VDDA = 3300 mV.

Real supply/reference voltage may differ.

## 22. Extension Exercises

- multi-channel scan;
- low-pass filter;
- RMS/variance;
- calibrated VDDA;
- no-copy ping-pong design;
- queue multiple blocks;
- stream results over UART.

## 23. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
