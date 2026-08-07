# 08-adc-dma

Register-level bare-metal ADC + DMA example for the STM32F103C8T6 Blue Pill.

## Hardware

Connect a potentiometer:

```text
3.3 V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

Do not drive PA0 below GND or above 3.3 V.

## Data path

```text
TIM3 update @ 1 kHz
        |
        v
ADC1 channel 0 / PA0
        |
        v
DMA1 Channel 1 circular
        |
        +-- half transfer --> 32-sample block
        |
        +-- full transfer --> 32-sample block
        |
        v
ADC Service
        |
        +-- average / minimum / maximum
        +-- estimated millivolts
        |
        v
Application
        |
        +-- PC13 LED hysteresis
```

- ADC resolution: 12 bit
- Reference assumption: 3300 mV
- ADC clock: at or below 12 MHz
- Sample time: 55.5 ADC cycles
- Trigger: TIM3 TRGO update
- Sample rate: 1000 samples/s
- DMA buffer: 64 x uint16_t
- Published block: 32 samples
- DMA mode: circular
- DMA IRQ: half transfer, transfer complete, transfer error

`TIM3_IRQHandler` and `ADC1_2_IRQHandler` are not used. Only
`DMA1_Channel1_IRQHandler` is strong.

## LED thresholds

The onboard PC13 LED uses hysteresis:

- turn on at or above 1800 mV
- turn off at or below 1500 mV

## Debug values

```gdb
p application_adc_average_raw
p application_adc_minimum_raw
p application_adc_maximum_raw
p application_adc_millivolts
p application_adc_sequence
p application_adc_dma_overruns
p application_adc_dma_errors
```

Typical raw values:

```text
0.0 V  -> about 0
1.65 V -> about 2048
3.3 V  -> about 4095
```

The millivolt calculation assumes VDDA is exactly 3300 mV.

## Build

```sh
make check-layers
make clean
make
make flash
```

`system_idle()` intentionally uses `NOP`, not `WFI`, to keep SWD attach
predictable with ST-Link adapters that do not expose NRST.
