# 01-blink-led — GPIO output + SysTick + non-blocking super-loop

Example đầu tiên của chuỗi register-level. Firmware cấu hình clock, điều khiển LED onboard **PC13** và dùng **SysTick 1 kHz** làm timebase để toggle LED mỗi **500 ms** mà không busy-delay trong Application.

## 1. Mục tiêu học tập

Sau example này cần nắm được:

- vector table và `Reset_Handler` hoạt động như thế nào,
- `.data` được copy và `.bss` được zero trước `main()`,
- cách RCC chuyển từ HSI sang HSE + PLL,
- cách cấu hình GPIO output bằng `CRH`/`BSRR`,
- cách cấu hình SysTick trực tiếp qua core register,
- cách viết periodic task bằng timestamp thay vì delay blocking,
- cách tách Application → Service → BSP → MCAL → Platform,
- cách dùng `make check-layers` để ngăn include sai tầng.

Không dùng HAL, LL, SPL, libopencm3, Arduino Core hoặc RTOS.

## 2. Kết quả mong đợi

LED onboard Blue Pill tại PC13 toggle mỗi 500 ms:

```text
500 ms OFF
500 ms ON
500 ms OFF
...
```

PC13 là active-low:

```text
PC13 = 0 → LED ON
PC13 = 1 → LED OFF
```

Cấu hình mặc định:

| Tham số | Giá trị |
|---|---:|
| HSE | 8 MHz |
| SYSCLK mục tiêu | 72 MHz |
| Fallback | HSI 8 MHz |
| SysTick | 1000 Hz |
| Tick period | 1 ms |
| LED | PC13 |
| LED active level | Low |
| Blink period | 500 ms |

## 3. Phần cứng

Không cần linh kiện ngoài ngoài Blue Pill và debugger.

SWD:

```text
ST-Link             Blue Pill
-----------------------------
SWDIO      -------  PA13
SWCLK      -------  PA14
GND        -------  GND
3.3V REF   -------  3.3V
```

## 4. Cấu hình compile-time

`config/board_config.h`:

```c
#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TARGET_CLOCK_HZ   (72000000UL)
#define BOARD_TIMEBASE_HZ       (1000UL)
```

`config/application_config.h`:

```c
#define APPLICATION_BLINK_PERIOD_MS (500UL)
```

`config/mcal_config.h`:

```c
#define MCAL_RCC_READY_TIMEOUT_CYCLES (1000000UL)
```

`config/service_config.h` hiện giữ event queue capacity 16 phần tử. Event service được init như một phần skeleton service nhưng blink logic hiện tại không publish event.

## 5. Luồng khởi động

```text
Reset_Handler
    ↓
runtime_init()
    ├─ copy .data Flash → RAM
    ├─ zero .bss
    └─ main()
         ↓
cortex_m3_disable_irq()
         ↓
system_init()
    ├─ board_init()
    │   ├─ configure HSE/PLL hoặc HSI fallback
    │   ├─ board_led_init()
    │   └─ board_timebase_init()
    ├─ time_service_init()
    ├─ indication_service_init()
    ├─ event_service_init()
    └─ application_init()
         ↓
cortex_m3_enable_irq()
         ↓
super-loop
```

## 6. Clock setup

`mcal_rcc_configure_hse_pll()` thực hiện:

1. bảo đảm HSI đang chạy,
2. switch system clock về HSI trước khi đổi PLL,
3. tắt PLL và chờ `PLLRDY` clear,
4. bật HSE và chờ `HSERDY`,
5. chọn Flash latency theo clock mục tiêu,
6. cấu hình APB1 prescaler khi cần,
7. chọn HSE làm PLL source và multiplier,
8. bật PLL, chờ `PLLRDY`,
9. switch SYSCLK sang PLL,
10. ghi lại `g_system_clock_hz`.

Nếu HSE/PLL fail, `mcal_rcc_use_hsi()` đưa hệ thống về HSI 8 MHz.

## 7. GPIO LED

Mapping:

```c
#define BOARD_STATUS_LED_PORT         MCAL_GPIO_PORT_C
#define BOARD_STATUS_LED_PIN          (13U)
#define BOARD_STATUS_LED_ACTIVE_LEVEL MCAL_GPIO_LEVEL_LOW
```

Luồng:

```text
Application
  → indication_service_toggle()
  → board_led_toggle()
  → mcal_gpio_toggle()
  → GPIOC ODR/BSRR
```

BSP chịu trách nhiệm active-low; Application chỉ biết logical `INDICATION_STATUS`.

## 8. SysTick timebase

Luồng thời gian:

```text
SysTick_Handler
    ↓
g_systick_ticks++
    ↓
board_timebase_now_ms()
    ↓
time_service_now_ms()
```

Với `BOARD_TIMEBASE_HZ = 1000`, mỗi tick tương ứng 1 ms.

`time_service_periodic_due()` dùng subtraction trên `uint32_t`, nên pattern vẫn hoạt động đúng qua wrap-around miễn period nhỏ hơn vùng ambiguity của counter.

## 9. Application state

Application chỉ cần một timestamp:

```c
static uint32_t g_last_blink_ms;
```

Logic:

```text
now - last >= 500 ms?
    ├─ no  → return
    └─ yes → update last
             toggle indication
```

Không có vòng `for` delay hoặc wait loop trong `application_process()`.

## 10. Kiến trúc

```text
Application
   ├──────────────→ Time Service → Board Timebase → MCAL SysTick
   └──────────────→ Indication Service → Board LED → MCAL GPIO
                                                   ↓
                                               Platform
```

Application không include `mcal_gpio.h`, `mcal_systick.h` hoặc STM32 register header.

## 11. Interrupt

Strong interrupt liên quan example:

```text
SysTick_Handler
```

ISR chỉ tăng tick counter. Blink decision vẫn chạy ở thread mode.

Các fault handler mạnh (`NMI`, `HardFault`, `MemManage`, `BusFault`, `UsageFault`) đi vào `system_panic()`.

## 12. Idle và panic

Numbered example dùng:

```c
void system_idle(void)
{
    cortex_m3_nop();
}
```

`system_panic()` disable IRQ rồi lặp `NOP`. Đây là lựa chọn debug-friendly cho ST-Link không có NRST; không phải power-saving mode.

## 13. File nên đọc theo thứ tự

```text
app/src/application.c
services/src/time_service.c
services/src/indication_service.c
bsp/bluepill/src/board.c
bsp/bluepill/src/board_led.c
bsp/bluepill/src/board_timebase.c
mcal/src/mcal_rcc.c
mcal/src/mcal_gpio.c
mcal/src/mcal_systick.c
platform/device/stm32f103xb/
system/system_init.c
startup/
linker/
```


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


Debug gợi ý:

```gdb
break SysTick_Handler
continue
```

Không nên để breakpoint ISR quá lâu nếu muốn quan sát timing thực tế; có thể halt sau vài giây và inspect `g_systick_ticks` nếu debug symbol cho phép.


## 14. Troubleshooting

### LED không blink

Kiểm tra:

- board có nguồn và GND chung với ST-Link,
- firmware flash thành công,
- PC13 đúng là LED onboard của board,
- `SysTick_Handler` là strong symbol,
- `system_init()` không rơi vào panic,
- HSE fail vẫn phải fallback HSI, nên blink vẫn có thể hoạt động.

### LED luôn sáng hoặc luôn tắt

Kiểm tra active-low mapping và `board_led.c`. Application không nên tự đảo logic active-low.

### Debug khó attach

Giữ `system_idle()` bằng `NOP`, OpenOCD `reset_config none`, giảm adapter speed nếu wiring dài/xấu.

## 15. Bài tập mở rộng

- thay đổi blink period runtime,
- thêm nhiều software timer,
- thêm event để đổi blink mode,
- thêm watchdog,
- thêm UART log clock source,
- chuyển idle sang `WFI` và đánh giá ảnh hưởng debug/power.

## 16. Tài liệu liên quan

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
