# Porting Guide — 01-blink-led

## 1. Mục tiêu port

Giữ nguyên Application:

```text
time_service_periodic_due()
indication_service_toggle()
```

và thay phần board/hardware bên dưới.

## 2. Port sang Blue Pill khác cùng STM32F103

Nếu LED vẫn PC13 và crystal vẫn 8 MHz thì không cần thay behavior. Chỉ kiểm tra:

- board variant có LED active-low hay không,
- HSE thực tế có 8 MHz,
- SWD wiring.

## 3. Đổi LED sang pin khác

Sửa:

```text
bsp/bluepill/include/board_pins.h
```

Ví dụ:

```c
#define BOARD_STATUS_LED_PORT MCAL_GPIO_PORT_B
#define BOARD_STATUS_LED_PIN  (0U)
```

Nếu active-high, đổi `BOARD_STATUS_LED_ACTIVE_LEVEL`.

Không sửa `application.c`.

## 4. Đổi system clock

Sửa:

```c
BOARD_HSE_FREQUENCY_HZ
BOARD_TARGET_CLOCK_HZ
```

Sau đó kiểm tra:

- PLL multiplier có hợp lệ với implementation hiện tại,
- target không vượt giới hạn STM32F103,
- Flash latency,
- APB1 limit,
- SysTick reload chia hết cho `BOARD_TIMEBASE_HZ`.

## 5. Đổi timebase frequency

`time_service` hiện hiểu tick là milliseconds vì config enforce 1 kHz. Nếu muốn 100 Hz hoặc 10 kHz, cần thay contract BSP/Service hoặc conversion logic, không chỉ đổi macro.

## 6. Đổi sang timer thay SysTick

Tạo:

```text
mcal_timer_timebase.*
board_timebase.*
```

Giữ API:

```c
uint32_t board_timebase_now_ms(void);
```

Application và Time Service có thể giữ nguyên.

## 7. Port sang MCU STM32F1 khác

Cần rà:

- linker Flash/RAM size,
- vector table/IRQ count,
- GPIO register layout,
- RCC PLL/clock tree,
- peripheral base addresses.

Nếu vẫn Cortex-M3, phần architecture core có thể tái sử dụng đáng kể.

## 8. Port sang MCU family khác

Thay:

```text
platform/device/
mcal/
startup/
linker/
```

Có thể cần đổi cả `platform/arch/` nếu core không phải Cortex-M3.

## 9. Validation checklist

Sau port:

```bash
make check-layers
make clean
make
make size
```

Kiểm tra runtime:

- `main` được hit bằng GDB,
- SysTick ISR chạy,
- LED default OFF đúng polarity,
- toggle period đúng,
- HSE fallback không làm firmware treo,
- fault path vẫn debug được.

## 10. Sai lầm thường gặp

- hard-code `GPIOC` trong Service,
- đổi pin nhưng quên clock enable port,
- đổi clock nhưng giữ SysTick reload cũ,
- đảo active-low ở Application,
- dùng delay loop dựa trên CPU clock,
- bỏ layer checker vì project nhỏ.

## 11. Mục tiêu sau khi port

Nếu Application và Services không đổi mà firmware vẫn blink đúng, port boundary đã được giữ tốt.
