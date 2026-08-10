# 05-timer-pwm — TIM2 Channel 1 Hardware PWM

## 1. Learning Objectives

This example introduces hardware PWM generated directly by TIM2.

You will learn:

- PA0 alternate-function output;
- TIM2_CH1 configuration;
- APB1 timer-clock behavior;
- PSC/ARR/CCR calculations;
- preload behavior;
- duty representation in permille;
- separating high-frequency waveform generation from low-frequency
  Application scheduling.

## 2. Wiring

Use an external LED:

```text
PA0 / TIM2_CH1 ---- 330 ohm ---- LED anode
GND --------------------------- LED cathode
```

The onboard PC13 LED is not used for PWM.

## 3. Configuration

```c
#define BOARD_HSE_FREQUENCY_HZ  (8000000UL)
#define BOARD_TARGET_CLOCK_HZ    (72000000UL)
#define BOARD_TIMEBASE_HZ        (1000UL)

#define BOARD_PWM_TIMER_TICK_HZ  (1000000UL)
#define BOARD_PWM_FREQUENCY_HZ   (1000UL)

#define APPLICATION_PWM_UPDATE_PERIOD_MS  (10UL)
#define APPLICATION_PWM_STEP_PERMILLE     (10U)
```

The intended breathing cycle is roughly:

```text
0% -> 100% in about 1 second
100% -> 0% in about 1 second
```

## 4. Clock Calculation

TIM2 is on APB1.

At the normal 72 MHz system clock:

```text
PCLK1 = 36 MHz
TIM2 clock = 72 MHz
```

because STM32F1 doubles the timer clock when the APB prescaler is not 1.

Target timer tick:

```text
1 MHz
```

Therefore:

```text
PSC = 72 MHz / 1 MHz - 1 = 71
```

PWM period:

```text
1 MHz / 1 kHz = 1000 counts
ARR = 999
```

Under the 8 MHz HSI fallback:

```text
TIM2 clock = 8 MHz
PSC = 7
ARR = 999
```

PWM frequency remains 1 kHz.

## 5. Duty Representation

Duty is represented in permille:

```text
0    -> 0%
500  -> 50%
1000 -> 100%
```

Application does not know the timer period count.

The MCAL/BSP converts permille into CCR1.

## 6. Timer Register Sequence

MCAL configures the relevant TIM2 registers:

```text
CR1
CCMR1
CCER
PSC
ARR
CCR1
EGR
```

Conceptual order:

1. disable timer while configuring;
2. set prescaler;
3. set auto-reload;
4. configure Channel 1 PWM mode;
5. enable preload;
6. configure output polarity/enable;
7. generate update event to load prescaler/preload state;
8. start timer.

## 7. Preload

ARR and CCR preload allow new values to become active at a timer update boundary.

Without preload, changing compare mid-period can produce one malformed pulse.

Preload keeps PWM transitions deterministic.

## 8. Application Fade Algorithm

Application state contains:

- current duty;
- increasing/decreasing direction;
- last update timestamp.

Every 10 ms:

```text
increasing?
    |
    +--> duty += 10
    +--> at 1000 -> reverse

decreasing?
    |
    +--> duty -= 10
    +--> at 0 -> reverse
```

## 9. Scheduling

SysTick provides the 1 ms Application timebase.

TIM2 produces PWM entirely in hardware.

The CPU only updates duty every 10 ms.

This means PWM edge timing does not depend on super-loop jitter.

## 10. Architecture

```text
Application
    |
    +--> PWM Service ------> Board PWM ------> MCAL Timer/GPIO
    |
    +--> Time Service -----> Board Timebase -> MCAL SysTick
```

## 11. Interrupts

TIM2 interrupts are not used.

Only SysTick is active for scheduling.

The PWM waveform continues even while the CPU is executing unrelated thread-mode
code.

## 12. Debug Symbols

Useful state:

```text
current duty
fade direction
last update timestamp
timer period counts
```

Useful breakpoints:

```gdb
break application_process
break board_pwm_set_duty_permille
```

## 13. Idle Behavior

`system_idle()` uses `cortex_m3_nop()`.

PWM generation continues independently in TIM2 hardware.

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

## 14. Test with an Oscilloscope/Logic Analyzer

Probe PA0.

Expected:

```text
frequency: about 1 kHz
period: about 1 ms
duty: slowly ramps 0% -> 100% -> 0%
```

The LED should visibly breathe.

## 15. Troubleshooting

### No Waveform

Check GPIO AF mode, TIM2 clock enable, Channel 1 enable, timer start, and wiring.

### Frequency Is Off by a Factor of Two

Check the APB1 timer x2 rule.

A common mistake is using PCLK1 directly as TIM2 clock when the APB1 prescaler
is greater than one.

### Duty Does Not Change

Check Application scheduling, SysTick, PWM Service calls, and CCR1 updates.

### LED Is Dim or the Fade Is Hard to See

Check LED orientation/resistor.

Human brightness perception is nonlinear, so a linear duty ramp is not a
perceptually linear brightness ramp.

## 16. Extension Exercises

- gamma-correct the fade;
- change PWM carrier frequency;
- use another timer/channel;
- drive RGB LEDs;
- use a sine lookup table;
- add multiple independent PWM outputs.

## 17. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
