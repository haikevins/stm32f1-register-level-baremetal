# Porting Guide — 02-gpio-input-interrupt

## 1. Moving the Button to Another Pin on STM32F103

Update together:

- BSP port;
- pin;
- active level;
- EXTI line;
- AFIO mapping;
- IRQ mapping if the line moves to a grouped handler.

Do not change Application.

## 2. Changing Polarity

For an active-high button:

- choose an appropriate pull-down/bias;
- change active level;
- configure rising edge instead of falling edge if desired.

Button Service can remain unchanged.

## 3. Using an External Pull-Up

Configure the GPIO as a suitable input mode without the internal pull-up.

Keep the physical idle level stable.

## 4. Changing Debounce

Change:

```c
BUTTON_SERVICE_DEBOUNCE_TIME_MS
```

Do not replace the timestamp algorithm with a blocking delay.

## 5. Changing IRQ Priority

Review the full interrupt-priority plan.

Higher priority does not permit longer ISR work.

## 6. Porting to Another STM32F1

Verify:

- AFIO register layout;
- EXTI line mapping;
- NVIC IRQ number;
- GPIO register differences;
- startup vector.

## 7. Porting to Another MCU Family

Preserve:

```text
raw edge -> Button Service -> Event Service -> Application
```

Replace BSP/MCAL/Platform implementation.

## 8. Verification Checklist

- [ ] idle input level stable;
- [ ] correct edge produces interrupt;
- [ ] pending flag clears;
- [ ] raw edge reaches Button Service;
- [ ] 30 ms debounce works;
- [ ] one press creates one logical event;
- [ ] LED toggles through Indication Service.

## 9. GDB Checklist

Break at:

```gdb
break EXTI0_IRQHandler
break button_service_process
break application_process
```

Inspect raw pending state and Service event state.

## 10. Logic Analyzer/Oscilloscope

Probe PA0.

You should see multiple fast bounce transitions but only one logical toggle.

## 11. Common Mistakes

- wrong AFIO port mapping;
- wrong EXTI line;
- missing pull resistor;
- wrong active polarity;
- clearing EXTI incorrectly;
- performing debounce in ISR;
- letting Application access EXTI directly.
