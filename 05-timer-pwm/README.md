# 05-timer-pwm — TIM2 Channel 1 hardware PWM

Example này dùng **TIM2_CH1 trên PA0** để tạo PWM phần cứng 1 kHz. Application cập nhật duty theo dạng tam giác 0% → 100% → 0% mỗi 10 ms để tạo hiệu ứng fade/breathing trên LED ngoài.

TIM2 tự tạo waveform bằng phần cứng; **TIM2 interrupt không được dùng**. SysTick 1 kHz chỉ dùng để schedule thay đổi duty.

## 1. Mục tiêu học tập

Example tập trung vào:

- clock tree APB1 và quy tắc timer clock x2,
- GPIO alternate-function push-pull,
- timer prescaler `PSC`,
- auto-reload `ARR`,
- compare register `CCR1`,
- PWM mode 1,
- preload `OC1PE` và `ARPE`,
- update event `UG`,
- biểu diễn duty theo permille,
- tách waveform generation khỏi Application scheduling.

## 2. Wiring

LED onboard PC13 không phải timer PWM channel trong mapping này. Dùng LED ngoài:

```text
PA0 / TIM2_CH1 ---- 330 Ω ---- LED anode
GND ------------------------- LED cathode
```

Nếu LED không sáng, kiểm tra chiều LED.

PWM là active-high.

## 3. Configuration

`config/board_config.h`:

```c
#define BOARD_HSE_FREQUENCY_HZ  (8000000UL)
#define BOARD_TARGET_CLOCK_HZ    (72000000UL)
#define BOARD_TIMEBASE_HZ        (1000UL)

#define BOARD_PWM_TIMER_TICK_HZ  (1000000UL)
#define BOARD_PWM_FREQUENCY_HZ   (1000UL)
```

`config/application_config.h`:

```c
#define APPLICATION_PWM_UPDATE_PERIOD_MS (10UL)
#define APPLICATION_PWM_STEP_PERMILLE    (10U)
```

Pin:

```c
#define BOARD_PWM_PORT MCAL_GPIO_PORT_A
#define BOARD_PWM_PIN  (0U)
```

## 4. Clock calculation

Với SYSCLK 72 MHz:

```text
APB1 = 36 MHz
```

Trên STM32F1, khi APB1 prescaler khác 1:

```text
TIM2 clock = 2 × PCLK1 = 72 MHz
```

Mục tiêu timer tick 1 MHz:

```text
PSC divider = 72 MHz / 1 MHz = 72
PSC = 72 - 1 = 71
```

Mục tiêu PWM 1 kHz:

```text
period counts = 1 MHz / 1 kHz = 1000
ARR = 1000 - 1 = 999
```

Do đó:

```text
PWM period = 1 ms
```

Trong HSI fallback 8 MHz:

```text
PCLK1 = 8 MHz
TIM2 clock = 8 MHz
PSC = 7
ARR = 999
```

PWM vẫn 1 kHz vì BSP truyền timer clock thực tế.

## 5. Duty representation

Public Service API dùng permille:

```text
0    = 0.0%
100  = 10.0%
500  = 50.0%
1000 = 100.0%
```

MCAL tính:

```text
period_counts = ARR + 1
CCR1 = period_counts × duty_permille / 1000
```

Với period 1000 count:

```text
duty 0    → CCR1 = 0
duty 500  → CCR1 = 500
duty 1000 → CCR1 = 1000
```

Trong PWM mode 1, `CCR1 = period_counts` cho output luôn active trong toàn vùng counter, tương ứng 100%.

## 6. Timer register sequence

MCAL:

1. enable TIM2 clock,
2. clear/disable timer config cũ,
3. ghi `PSC`, `ARR`, `CNT`,
4. set `OC1M = 110` (PWM mode 1),
5. set `OC1PE`,
6. ghi duty đầu,
7. enable output `CC1E`,
8. set `ARPE`,
9. generate `UG` để load shadow values,
10. clear `SR`,
11. set `CEN`.

Logic register:

```text
TIM2_CCMR1.OC1M = PWM1
TIM2_CCMR1.OC1PE = 1
TIM2_CCER.CC1E = 1
TIM2_CR1.ARPE = 1
TIM2_EGR.UG = 1
TIM2_CR1.CEN = 1
```

## 7. Preload

`ARR` và `CCR1` dùng preload. Khi Application đổi duty, compare value mới được latch theo update event thay vì thay đổi giữa một PWM period không kiểm soát.

Điều này giảm glitch khi duty thay đổi.

## 8. Application fade algorithm

Application expose:

```c
application_pwm_duty_permille
application_pwm_update_count
application_pwm_ramping_up
```

State machine:

```text
ramping_up = true:
    duty += 10
    tới 1000 → đổi direction

ramping_up = false:
    duty -= 10
    tới 0 → đổi direction
```

Mỗi step 10 ms.

Số step 0 → 1000:

```text
1000 / 10 = 100 step
100 × 10 ms = ~1 s
```

Chu kỳ lên-xuống khoảng 2 s.

## 9. Scheduling

`time_service_periodic_due()` dùng SysTick:

```text
SysTick 1 ms
    ↓
Time Service
    ↓
Application every 10 ms
    ↓
PWM Service
    ↓
TIM2 CCR1
```

TIM2 không cần interrupt để tạo PWM.

## 10. Kiến trúc

```text
Application
   ├─> Time Service
   │      ↓
   │   Board Timebase
   │      ↓
   │   MCAL SysTick
   │
   └─> PWM Service
          ↓
       Board PWM
          ↓
       MCAL Timer + GPIO
          ↓
       TIM2 / GPIOA
```

## 11. Interrupts

Strong handler:

```text
SysTick_Handler
```

Weak/default:

```text
TIM2_IRQHandler
```

Đây là điểm kiểm tra quan trọng: PWM generation hoàn toàn hardware.

## 12. Debug symbols

```gdb
p application_pwm_duty_permille
p application_pwm_update_count
p application_pwm_ramping_up
```

Expected duty khi halt ở các thời điểm khác nhau:

```text
0, 10, 20, ... 990, 1000, 990, ...
```

Có thể inspect `TIM2->CCR1` nếu debug lower layer.

## 13. Idle behavior

`system_idle()` dùng `NOP`. PWM vẫn chạy phần cứng kể cả CPU không ghi register; SysTick vẫn interrupt và Application tiếp tục schedule.


## Build, flash và debug

Yêu cầu công cụ:

```text
arm-none-eabi-gcc
arm-none-eabi-objcopy
arm-none-eabi-objdump
arm-none-eabi-size
GNU Make
Python 3
OpenOCD
arm-none-eabi-gdb hoặc gdb-multiarch
```

Build:

```bash
make check-layers
make clean
make
```

Các artifact chính:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

Flash:

```bash
make flash
```

Debug bằng hai terminal:

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

OpenOCD config dùng SWD, `reset_config none` và adapter speed 1000 kHz.


Breakpoint gợi ý:

```gdb
break mcal_timer_pwm_set_duty_permille
continue
```

Hoặc chỉ halt định kỳ và xem các global debug.


## 14. Test với oscilloscope/logic analyzer

Probe PA0:

Expected:

```text
frequency ≈ 1 kHz
period ≈ 1 ms
duty thay đổi chậm theo ~2 s cycle
```

Tại 50%:

```text
HIGH ≈ 0.5 ms
LOW  ≈ 0.5 ms
```

Đo frequency là cách tốt nhất để xác minh APB timer clock/prescaler.

## 15. Troubleshooting

### Không có waveform

Kiểm tra:

- PA0 AF push-pull,
- TIM2 clock enable,
- `CC1E`,
- `CEN`,
- pin không bị cấu hình lại sau init.

### Frequency sai x2

Rất thường do quên quy tắc timer clock x2 khi APB prescaler != 1.

### Duty không đổi

Kiểm tra:

- SysTick ISR,
- `application_pwm_update_count`,
- Service gọi Board PWM,
- `CCR1`,
- preload/update event.

### LED mờ/không thấy fade

LED/human perception phụ thuộc điện trở, loại LED, ambient light. Dùng oscilloscope để xác nhận PWM trước.

## 16. Bài tập mở rộng

- PWM channel khác,
- servo pulse 50 Hz,
- complementary waveform trên advanced timer,
- gamma correction cho LED,
- waveform table,
- timer interrupt thay SysTick scheduling,
- DMA update CCR cho waveform.

## 17. Tài liệu liên quan

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
