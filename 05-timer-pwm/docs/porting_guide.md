# Porting Guide — 05-timer-pwm

## 1. Changing Pin/Channel on the Same Timer

Select a valid TIM2 channel/pin mapping.

Update BSP and, if required, channel-specific MCAL operations.

## 2. Moving to Another Timer

Review:

- base address;
- RCC enable;
- APB bus;
- timer input clock;
- channel register mapping;
- IRQ only if needed.

## 3. Changing PWM Frequency

Change:

```c
BOARD_PWM_FREQUENCY_HZ
```

Verify:

```text
timer_tick % pwm_frequency == 0
```

and the resulting period fits the timer width.

## 4. Changing Timer Resolution

Changing the 1 MHz timer tick affects:

- prescaler;
- period resolution;
- maximum representable period.

Validate all ranges.

## 5. Changing Fade Speed

Adjust:

```text
APPLICATION_PWM_UPDATE_PERIOD_MS
APPLICATION_PWM_STEP_PERMILLE
```

Approximate one-way ramp:

```text
1000 / step * update_period
```

## 6. Active-Low PWM

Handle electrical polarity in BSP/MCAL output configuration.

Keep logical duty semantics unchanged.

## 7. Porting to Another MCU Family

Keep Application/PWM Service.

Replace timer/GPIO MCAL and Platform Device.

Re-check the timer-clock rule because another family may differ from STM32F1.

## 8. Validation Checklist

- [ ] timer clock derived correctly;
- [ ] carrier frequency correct;
- [ ] 0% and 100% correct;
- [ ] duty updates smoothly;
- [ ] preload works;
- [ ] no unnecessary timer interrupt;
- [ ] HSI fallback still produces correct frequency if retained.

## 9. Common Pitfalls

- ignoring APB timer x2;
- using wrong channel/pin;
- ARR off by one;
- period outside 16-bit range;
- changing clock tree without recalculating timer setup;
- leaking timer details into Application.
