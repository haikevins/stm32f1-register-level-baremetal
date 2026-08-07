# Porting guide

Board-specific assumptions are isolated in `bsp/bluepill`:

- ADC input: PA0 / ADC1 channel 0
- status LED: PC13, active low
- DMA mapping: ADC1 -> DMA1 Channel 1
- trigger timer: TIM3 TRGO

To port the example to another board using the same STM32F103 device, update
the board pin mapping and board composition while keeping Application and
Services unchanged.
