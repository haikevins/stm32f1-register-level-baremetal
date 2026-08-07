# 05-timer-pwm

Register-level bare-metal TIM2 PWM example for the STM32F103C8T6 Blue Pill.

TIM2 channel 1 drives PA0 with a 1 kHz hardware PWM waveform. The application
updates the duty cycle every 10 ms to create a triangular breathing/fade
pattern. TIM2 does not use interrupts; SysTick provides the 1 ms software
timebase.

## Wiring

Use an external LED because the Blue Pill onboard LED on PC13 is not connected
to a timer PWM channel:

```text
PA0 / TIM2_CH1 ---- 330 ohm ---- LED anode
GND ---------------------------- LED cathode
```

The PWM output is active high.

## Configuration

```text
Timer:             TIM2
Channel:           CH1
Pin:               PA0
Timer tick:        1 MHz
PWM frequency:     1 kHz
Duty range:        0..1000 permille
Duty update:       every 10 ms
Breathing step:    10 permille
TIM2 interrupt:    not used
```

At the normal 72 MHz clock:

```text
TIM2 input clock = 72 MHz
PSC              = 72 - 1
timer tick       = 1 MHz
ARR              = 1000 - 1
PWM frequency    = 1 kHz
```

TIM2 still works in the 8 MHz HSI fallback configuration. The BSP passes the
actual APB1 timer clock to MCAL so the prescaler is recalculated.

## Architecture

```text
Application
    |
    +--> PWM Service --> Board PWM --> MCAL Timer --> TIM2 registers
    |
    +--> Time Service -> Board Timebase -> MCAL SysTick -> Cortex-M3 SysTick
```

Application code does not include BSP, MCAL, platform or device-register
headers.

## PWM register setup

MCAL configures TIM2 channel 1 in PWM mode 1:

```text
RCC_APB1ENR.TIM2EN = 1
GPIOA pin 0        = alternate-function push-pull

TIM2_PSC           = timer_clock / 1 MHz - 1
TIM2_ARR           = 1000 - 1
TIM2_CCMR1.OC1M    = PWM mode 1
TIM2_CCMR1.OC1PE   = 1
TIM2_CCER.CC1E     = 1
TIM2_CR1.ARPE      = 1
TIM2_EGR.UG        = 1
TIM2_CR1.CEN       = 1
```

Duty cycle is changed by writing `TIM2_CCR1`. CCR1 preload means the new value
takes effect cleanly at the next timer update event.

## Debug variables

```gdb
p application_pwm_duty_permille
p application_pwm_update_count
p application_pwm_ramping_up
```

`application_pwm_duty_permille` moves between 0 and 1000.

You can also inspect the hardware compare register:

```gdb
p/x ((stm32_timer_registers_t *)0x40000000)->CCR1
```

## Build

```bash
make check-layers
make clean
make
```

## Flash

```bash
make flash
```

## Idle behavior

`system_idle()` executes `NOP`, not `WFI`. This keeps SWD attachment reliable
with an ST-Link setup that does not expose NRST. TIM2 continues generating PWM
independently of the CPU.
