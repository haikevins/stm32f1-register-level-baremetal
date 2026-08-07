# Architecture

```text
Application
    |
    +-------> ADC Service ---------> Board ADC DMA
    |                                  |
    |                                  +--> MCAL ADC
    |                                  +--> MCAL DMA
    |                                  +--> MCAL Timer Trigger
    |                                  +--> MCAL NVIC
    |
    +-------> Indication Service --> Board LED --> MCAL GPIO

MCAL --> STM32F103 register definitions
```

The DMA interrupt handler publishes completed 32-sample blocks but performs no
averaging, voltage conversion, LED logic, delay, or application work.

The application and services do not include STM32 register headers.
