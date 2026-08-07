# Architecture

## Dependency direction

```text
app -> services -> bsp -> mcal -> platform
```

`system` is the composition root.

## Responsibilities

### Application

- Schedules duty-cycle changes every 10 ms.
- Generates the triangular 0..1000 permille breathing waveform.
- Exposes duty, direction and update count for debugger inspection.

### PWM service

- Presents a board-independent duty-cycle API in permille.
- Clamps duty to the valid 0..1000 range.

### Time service

- Presents the 1 ms monotonic SysTick timebase to the application.

### BSP

- Maps PWM output to PA0 / TIM2_CH1.
- Initializes the board timebase.
- Passes the actual TIM2 clock to MCAL.

### MCAL

- Configures PA0 as alternate-function push-pull.
- Configures TIM2 PWM mode 1 through direct register access.
- Configures the Cortex-M3 SysTick timebase.
- Configures the STM32 clock tree and reports the APB1 timer clock.

### Platform

- Defines STM32F103 memory addresses, timer/RCC/GPIO register structures and
  bit masks.

## Interrupt policy

Only `SysTick_Handler` is strong. TIM2 runs without interrupts, so
`TIM2_IRQHandler` remains the weak default handler.
