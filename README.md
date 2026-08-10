# STM32F1 Register-Level Bare-Metal Examples

## 1. Philosophy of the Example Series

The examples are a progressive learning sequence.

Every project keeps the same architecture:

```text
Application -> Services -> BSP/ECUAL -> MCAL -> Platform -> Hardware
```

Only the peripheral topic changes.

The goal is not merely to make a peripheral work. The goal is to understand:

- which layer owns the peripheral;
- how clock/reset/configuration are performed;
- how an ISR hands work to thread mode;
- how Application remains hardware-independent.

## 2. Recommended Learning Roadmap

### Step 1 — GPIO and Timebase

`01-blink-led`

Learn:

- PC13 active-low output;
- RCC GPIO clock;
- SysTick;
- periodic non-blocking scheduling;
- logical indication Service.

### Step 2 — Interrupt Input

`02-gpio-input-interrupt`

Learn:

- PA0 pull-up;
- AFIO;
- EXTI0;
- NVIC;
- ISR event handoff;
- debounce outside ISR.

### Step 3 — UART Polling

`03-uart-polling`

Learn:

- USART1 registers;
- BRR calculation;
- RXNE/TXE polling;
- non-blocking byte API.

### Step 4 — UART Interrupt + Ring Buffer

`04-uart-interrupt-ring-buffer`

Learn:

- RX/TX ring buffers;
- single-producer/single-consumer ownership;
- USART IRQ;
- TXE interrupt lifecycle;
- overflow/error counters.

### Step 5 — Timer PWM

`05-timer-pwm`

Learn:

- TIM2_CH1;
- APB1 timer x2 clock behavior;
- PSC/ARR/CCR;
- preload;
- hardware PWM.

### Step 6 — I2C + External Device

`06-i2c-display`

Learn:

- I2C1 timing;
- PB6/PB7 AF open-drain;
- bounded polling;
- SSD1306 framebuffer;
- Service-to-ECUAL transport composition.

### Step 7 — SPI Flash

`07-spi-memory`

Learn:

- SPI1 register configuration;
- mode 0;
- software CS;
- W25Q64 JEDEC ID;
- WEL/BUSY;
- sector erase/page program/read-back.

### Step 8 — ADC + DMA Pipeline

`08-adc-dma`

Learn:

- TIM3 trigger;
- ADC1 calibration;
- DMA1 Channel 1 circular buffer;
- half/full-transfer events;
- stable block publishing;
- Service-side statistics;
- Application hysteresis.

## 3. Summary Table

| Example | Hardware | Interrupts | Main pattern |
|---|---|---|---|
| 01 | PC13 + SysTick | SysTick | periodic non-blocking task |
| 02 | PA0 + EXTI0 | SysTick, EXTI0 | raw edge -> debounce |
| 03 | USART1 | none | polling |
| 04 | USART1 | USART1 | RX/TX rings |
| 05 | TIM2_CH1 | SysTick | hardware PWM + scheduled duty |
| 06 | I2C1 + SSD1306 | SysTick | framebuffer + polling bus |
| 07 | SPI1 + W25Q64 | SysTick | synchronous memory commands |
| 08 | TIM3 + ADC1 + DMA1 CH1 | DMA1 CH1 | sampled-data blocks |

## 4. Wiring Summary

### SWD — Used by All Examples

```text
ST-Link      Blue Pill
----------------------
SWDIO   ---> PA13 / SWDIO
SWCLK   ---> PA14 / SWCLK
GND     ---> GND
3.3V    ---> 3.3V reference
```

### Example 02 — Button

```text
PA0 ---- push button ---- GND
```

PA0 uses the internal pull-up.

### Example 03/04 — USB-UART

```text
PA9  USART1_TX  ---> USB-UART RX
PA10 USART1_RX  <--- USB-UART TX
GND              --- USB-UART GND
```

Use 3.3 V logic, `115200 8N1`.

### Example 05 — PWM LED

```text
PA0 / TIM2_CH1 ---- 330 ohm ---- LED ---- GND
```

### Example 06 — OLED I2C

```text
Blue Pill      SSD1306
----------------------
3.3V       ---> VCC
GND        ---> GND
PB6        ---> SCL
PB7        ---> SDA
```

### Example 07 — W25Q64

```text
STM32F103C8T6       W25Q64
--------------------------------
3.3V        ------  VCC
GND         ------  GND
PA4         ------  CS
PA5         ------  CLK
PA6         ------  D1 / DO / MISO
PA7         ------  D0 / DI / MOSI
```

### Example 08 — Analog Input

```text
3.3 V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

## 5. Common Build/Flash Flow

```bash
make check-layers
make clean
make
make flash
```

Useful targets:

```bash
make size
make tree
make erase
```

## 6. Common Debug Flow

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

The OpenOCD configuration uses `reset_config none`.

## 7. Common Clock Behavior

Every board first tries 72 MHz HSE+PLL.

If that fails:

```text
HSI fallback = 8 MHz
```

Peripheral code must use the actual active clock.

Examples:

- SysTick reload follows active SYSCLK.
- TIM2/TIM3 calculations use actual APB1 timer clock.
- SPI1 prescaler uses actual PCLK2.
- I2C1 timing uses actual PCLK1.
- ADC clock selection stays within the configured maximum.

## 8. Architecture and Layer Checker

The register-level dependency path is:

```text
Application
    |
Services
    |
BSP / ECUAL
    |
MCAL
    |
Platform Device
    |
Platform Architecture
```

Run:

```bash
make check-layers
```

Do not bypass the checker to make a forbidden include compile.

## 9. Interrupt Ownership and Thread Mode

Interrupt handlers stay at the lowest owning layer.

The ISR normally:

```text
read/clear flag
capture byte/event/block
update bounded state
return
```

Debounce, echo policy, display rendering, memory verification, sample
statistics, and LED policy belong in thread mode.

## 10. `system_idle()` in the Example Series

The completed examples use:

```c
cortex_m3_nop();
```

rather than `WFI`.

This is intentional for a debug setup without NRST.

## 11. Choosing an Example to Extend

Choose the example whose **architecture pattern** matches your new problem:

- periodic task -> 01;
- edge/event input -> 02;
- polling byte stream -> 03;
- interrupt byte stream -> 04;
- hardware waveform -> 05;
- I2C external device -> 06;
- SPI memory/device -> 07;
- continuous sampled data -> 08.

## 12. Rules When Copying Code Between Examples

When reusing a module:

1. copy its public API;
2. copy its implementation;
3. copy required configuration;
4. copy required Platform/MCAL support;
5. preserve ISR ownership;
6. preserve clock assumptions;
7. run the layer checker;
8. re-test hardware.

Avoid copying isolated register writes without their initialization/error
context.

## 13. Hardware Safety Notes

- Use 3.3 V logic.
- Always share ground.
- Use a resistor with an external LED.
- Keep ADC input between GND and VDDA.
- Verify I2C pull-ups.
- Power W25Q64 from 3.3 V.
- Remember Example 07 erases the last 4 KiB sector at every reset.

## 14. Detailed Documentation

Every example contains:

```text
README.md
docs/architecture.md
docs/porting_guide.md
```

Use the README for bring-up, architecture document for ownership/concurrency,
and porting guide before changing pins, clocks, IRQs, or MCU family.
