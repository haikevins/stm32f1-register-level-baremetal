# Porting Guide — 05-timer-pwm

## 1. Đổi pin/channel cùng timer

Kiểm tra alternate-function mapping của STM32F103. Không phải mọi GPIO đều map được TIM2_CH1 nếu không remap.

Cập nhật:

```text
bsp/bluepill/include/board_pins.h
bsp/bluepill/src/board_pwm.c
```

Nếu cần AFIO remap, bổ sung BSP/MCAL support.

## 2. Đổi sang timer khác

Cần:

- base address,
- RCC enable bit,
- timer instance enum/mapping,
- channel-specific CCMR/CCER/CCR,
- actual timer input clock bus.

TIM2/3/4 nằm APB1; TIM1 nằm APB2 và có advanced-timer differences.

## 3. Đổi PWM frequency

Sửa:

```c
BOARD_PWM_FREQUENCY_HZ
```

Contract hiện tại yêu cầu:

```text
BOARD_PWM_TIMER_TICK_HZ % BOARD_PWM_FREQUENCY_HZ == 0
```

Nếu cần frequency không chia hết, mở rộng algorithm chọn PSC/ARR với error minimization.

## 4. Đổi timer resolution

`BOARD_PWM_TIMER_TICK_HZ` quyết định count resolution. Higher tick:

- resolution tốt hơn,
- có thể tăng requirement clock,
- ARR lớn hơn.

Phải giữ PSC/ARR trong 16-bit range của timer implementation.

## 5. Đổi fade speed

Application config:

```c
APPLICATION_PWM_UPDATE_PERIOD_MS
APPLICATION_PWM_STEP_PERMILLE
```

Không cần sửa timer waveform frequency.

## 6. Active-low PWM

Nếu load active-low, có thể:

- đảo output polarity bằng CCER nếu driver hỗ trợ,
- hoặc map semantic duty ở BSP/Service.

Không nên tự `1000-duty` rải rác trong Application nếu board polarity là hardware concern.

## 7. Port sang family khác

Giữ PWM Service/Application, thay timer MCAL/register map/BSP.

## 8. Validation checklist

- đo SCK? Không, đo trực tiếp PWM PA0,
- frequency đúng,
- 0/50/100% đúng,
- no glitches đáng kể khi update,
- HSI fallback vẫn đúng frequency,
- `TIM2_IRQHandler` weak,
- layer checker pass.

## 9. Common pitfalls

- quên timer x2 clock,
- ARR off-by-one,
- CCR 100% xử lý sai,
- preload không generate UG lúc init,
- pin không đúng AF,
- update duty bằng delay blocking.
