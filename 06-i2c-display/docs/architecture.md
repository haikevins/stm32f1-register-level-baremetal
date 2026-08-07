# Architecture

```text
Application
    |
    v
Display Service ---------> Board Display Bus
    |                           |
    v                           v
SSD1306 ECUAL              MCAL I2C / GPIO
                                |
                                v
                         STM32F103 registers
```

The service is the composition point for the display transport. ECUAL remains
independent of the board implementation by accepting write and delay callbacks.

SysTick provides the 1 ms timebase used by the application and the OLED
power-on delay. I2C1 is polling only; no I2C interrupt handler is required.
