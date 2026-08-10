# Architecture

```text
Application
    |
    v
Memory Service
   / \
  /   \
  v    v
W25Q64 ECUAL      Board Memory Bus
                       |
                 +-----+-----+
                 v           v
              MCAL SPI    MCAL GPIO
                 \           /
                  \         /
                   v       v
                STM32F103 registers

Application --> Indication Service --> Board LED --> MCAL GPIO
Application --> Time Service -------> Board Timebase --> MCAL SysTick
```

The W25Q64 ECUAL receives transfer/select/deselect callbacks from the Memory
Service. It therefore remains independent of the Blue Pill pin mapping and
STM32 register definitions.

SPI uses polling only. `SPI1_IRQHandler` remains the startup weak default
handler.
