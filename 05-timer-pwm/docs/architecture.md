# Architecture — 05-timer-pwm

## 1. Dependency Graph

```text
Application
    |
    +--> PWM Service ------> Board PWM ------> MCAL Timer/GPIO
    |
    +--> Time Service -----> Board Timebase -> MCAL SysTick
```

## 2. Ownership

Application owns the breathing waveform policy.

PWM Service owns the logical duty API.

BSP owns PA0/TIM2_CH1 mapping.

MCAL owns timer/GPIO registers and clock-to-register conversion.

## 3. Hardware vs Software Timing

Two time domains exist:

```text
1 kHz carrier -> TIM2 hardware
10 ms duty update -> thread mode
```

Only the second depends on the super-loop.

## 4. Why Permille at the Service Boundary

Permille is independent from:

- timer width;
- ARR value;
- input clock;
- target PWM frequency.

This keeps Application reusable.

## 5. Clock Ownership

Application never handles PCLK1 or the APB timer multiplier.

BSP/MCAL derive actual TIM2 clock from RCC state.

## 6. Preload Semantics

ARR/CCR preload ensures register updates become active at update events.

This avoids partial-cycle glitches.

## 7. ISR Policy

TIM2 does not need an ISR for hardware PWM.

Adding a timer ISR just to generate PWM would increase jitter and CPU load.

## 8. Failure Handling

Initialization validates clock divisibility and timer range.

Fatal initialization failure propagates to `system_panic()`.

## 9. Extension Boundary

Good boundaries:

- brightness pattern -> Application;
- logical duty -> Service;
- channel/pin -> BSP;
- timer registers -> MCAL.

## 10. What Not to Do

Avoid:

- writing CCR1 from Application;
- hard-coding 72 MHz in Application;
- software-toggling GPIO for PWM;
- performing the breathing state machine in a high-rate ISR.
