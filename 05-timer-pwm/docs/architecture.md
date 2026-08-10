# Kiến trúc — 05-timer-pwm

## 1. Dependency graph

```text
Application
   ├─> Time Service → Board Timebase → MCAL SysTick → Cortex-M3
   └─> PWM Service  → Board PWM      → MCAL Timer
                                         └─> MCAL GPIO
                                              ↓
                                           Platform
```

## 2. Ownership

| Concern | Owner |
|---|---|
| Fade direction/step | Application |
| Duty unit/clamp | PWM Service |
| PA0/TIM2_CH1 mapping | BSP |
| PSC/ARR/CCR/PWM mode | MCAL Timer |
| APB timer clock | MCAL RCC |
| 1 ms tick | MCAL SysTick |

## 3. Hardware vs software timing

Hai time domain độc lập:

```text
TIM2 = waveform generation 1 kHz
SysTick = application update scheduler 1 kHz tick
```

PWM không phụ thuộc việc Application chạy đúng từng microsecond. CPU chỉ cập nhật duty thưa hơn.

## 4. Why permille at Service boundary

Service không expose raw CCR:

- raw CCR phụ thuộc ARR,
- Application không cần biết timer resolution,
- 0..1000 portable hơn,
- dễ clamp.

Đây là ví dụ chuyển từ hardware unit sang semantic unit.

## 5. Clock ownership

Application không biết SYSCLK/PCLK1. BSP lấy actual timer clock từ RCC và truyền xuống MCAL. MCAL chỉ cần:

```text
timer_clock_hz
timer_tick_hz
pwm_frequency_hz
```

## 6. Preload semantics

MCAL bật `OC1PE` và `ARPE`; Service chỉ gọi set duty. Application không cần biết latch timing.

## 7. ISR policy

TIM2 không interrupt. SysTick ISR thuộc MCAL SysTick và chỉ tăng counter.

## 8. Failure handling

Nếu timer clock không chia hết timer tick hoặc tick không chia hết PWM frequency, MCAL init fail. `system_init()` propagate fail tới `system_panic()`.

Điều này tránh silently tạo frequency gần đúng ngoài contract hiện tại.

## 9. Extension boundary

- đổi waveform policy → Application,
- đổi unit/API → Service,
- đổi pin/timer channel → BSP,
- hỗ trợ timer/channel mới → MCAL/Platform,
- đổi clock tree → RCC.

## 10. Không nên làm

Không ghi `TIM2->CCR1` trong Application. Làm vậy buộc behavior phụ thuộc resolution/register và phá layer boundary.
