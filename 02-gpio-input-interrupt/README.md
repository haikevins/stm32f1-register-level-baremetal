# 02-gpio-input-interrupt — GPIO input + EXTI + debounce ngoài ISR

Example này mở rộng nền tảng của `01-blink-led` bằng một nút nhấn ngoài tại **PA0**. PA0 dùng pull-up nội, **EXTI0** bắt falling edge khi nhấn, ISR chỉ ghi nhận event, còn **Button Service** thực hiện debounce 30 ms ở thread mode. Một press hợp lệ sẽ toggle LED onboard **PC13**.

## 1. Mục tiêu học tập

Example minh họa:

- GPIO input pull-up trên STM32F1,
- mapping GPIO → EXTI qua AFIO,
- cấu hình falling-edge trigger,
- enable IRQ và priority trong NVIC,
- clear EXTI pending flag đúng semantics write-one-to-clear,
- cách ISR chuyển hardware event thành software event nhỏ gọn,
- debounce theo timestamp thay vì delay trong ISR,
- giữ Application không biết EXTI/GPIO register,
- debug interrupt với ST-Link/OpenOCD.

## 2. Wiring

Dùng push button normally-open:

```text
PA0 ---- push button ---- GND
```

Không cần điện trở pull-up ngoài vì firmware cấu hình pull-up nội.

LED onboard:

```text
PC13 = active-low
```

Bảng tài nguyên:

| Resource | Giá trị |
|---|---|
| Button | PA0 |
| GPIO mode | Input pull |
| Pull | Up |
| Active level | Low |
| EXTI line | 0 |
| Trigger | Falling |
| IRQ priority | 2 |
| Debounce | 30 ms |
| LED | PC13 active-low |
| Timebase | SysTick 1 kHz |

## 3. Behavior

Khi thả nút:

```text
PA0 = HIGH
```

Khi nhấn:

```text
PA0: HIGH → LOW
        ↓
      EXTI0
```

Sau một press hợp lệ:

```text
LED OFF → ON
LED ON  → OFF
```

Bounce điện cơ không được xử lý trong ISR. Mỗi falling edge mới chỉ đánh dấu/restart cửa sổ debounce; sau 30 ms Service đọc lại PA0.

## 4. Config

`config/service_config.h`:

```c
#define SERVICE_EVENT_QUEUE_CAPACITY    (16U)
#define BUTTON_SERVICE_DEBOUNCE_TIME_MS (30UL)
```

`bsp/bluepill/include/board_pins.h`:

```c
#define BOARD_USER_BUTTON_PORT         MCAL_GPIO_PORT_A
#define BOARD_USER_BUTTON_PIN          (0U)
#define BOARD_USER_BUTTON_ACTIVE_LEVEL MCAL_GPIO_LEVEL_LOW
#define BOARD_USER_BUTTON_EXTI_LINE    (0U)
#define BOARD_USER_BUTTON_IRQ_PRIORITY (2U)
```

Clock/timebase:

```c
#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TARGET_CLOCK_HZ   (72000000UL)
#define BOARD_TIMEBASE_HZ       (1000UL)
```

## 5. Initialization flow

```text
system_init()
  ├─ board_init()
  │   ├─ clock setup
  │   ├─ board_led_init()
  │   ├─ board_timebase_init()
  │   └─ board_button_init()
  │       ├─ PA0 input pull-up
  │       └─ EXTI0 falling + NVIC
  ├─ time_service_init()
  ├─ indication_service_init()
  ├─ button_service_init()
  │   └─ discard stale latched EXTI event
  ├─ event_service_init()
  └─ application_init()
```

Global IRQ chỉ được enable sau khi `system_init()` hoàn tất.

## 6. Register-level GPIO input pull-up

Trên STM32F1, input pull-up/pull-down dùng:

```text
GPIOx_CRL/CRH: CNF = input pull
GPIOx_ODR bit: 1 = pull-up, 0 = pull-down
```

BSP gọi:

```c
mcal_gpio_configure(
    PA0,
    INPUT_PULL,
    HIGH);
```

MCAL xử lý GPIO port clock và register field tương ứng.

## 7. AFIO + EXTI setup

MCAL thực hiện logic tương đương:

```text
RCC_APB2ENR.AFIOEN = 1
AFIO_EXTICR1.EXTI0 = Port A
EXTI_IMR.MR0 = 0 trong khi cấu hình
EXTI_RTSR.TR0 = 0
EXTI_FTSR.TR0 = 1
EXTI_PR.PR0 = 1 để clear pending cũ
NVIC priority cho EXTI0
NVIC clear pending
NVIC enable EXTI0
EXTI_IMR.MR0 = 1
```

## 8. ISR ownership

`EXTI0_IRQHandler()` nằm trong MCAL EXTI:

```c
void EXTI0_IRQHandler(void)
{
    record_pending_lines(UINT32_C(1) << 0U);
}
```

Handler chung:

1. đọc `EXTI->PR`,
2. mask line được quan tâm,
3. ghi lại bit pending vào `EXTI->PR` để clear,
4. OR bit vào `g_exti_events`.

ISR không:

- đọc time,
- debounce,
- toggle LED,
- gọi Application,
- delay,
- allocate.

## 9. Event handoff từ ISR sang thread mode

BSP:

```text
board_button_take_press_event()
    ↓
mcal_exti_take_event(0)
```

`mcal_exti_take_event()` dùng critical section ngắn để đọc-clear `g_exti_events` an toàn so với ISR.

Đây là pattern "record in ISR, consume in thread mode".

## 10. Debounce algorithm

State của Button Service:

```text
g_debounce_pending
g_debounce_started_ms
```

Flow:

```text
EXTI event?
  ├─ yes → debounce_pending = true
  │        debounce_started = now
  └─ no

debounce_pending?
  ├─ no → no press
  └─ yes
       ↓
elapsed < 30 ms?
  ├─ yes → no press
  └─ no
       ↓
read PA0 again
  ├─ LOW → valid press
  └─ HIGH → bounce/noise, discard
```

Nếu bounce tạo falling edge mới, timestamp được restart.

## 11. Application

Application rất nhỏ:

```text
button_service_take_press()
    ↓ true
indication_service_toggle()
```

Không có pin number, EXTI line hoặc debounce constant trong Application.

## 12. Kiến trúc

```text
PA0
 ↓
MCAL GPIO + EXTI
 ↓
Board Button
 ↓
Button Service
 ↓
Application
 ↓
Indication Service
 ↓
Board LED
 ↓
MCAL GPIO
 ↓
PC13
```

Time path:

```text
SysTick ISR → Time Service → Button Service
```

## 13. Event Service

Example vẫn init generic Event Service/static queue, nhưng button path hiện tại dùng MCAL event bit trực tiếp qua Board Button và không publish `service_event_t`. Đây là module scaffold còn lại để mở rộng event-driven design.

## 14. Idle behavior

`system_idle()` dùng `NOP`, không `WFI`. EXTI và SysTick vẫn chạy vì global interrupt đã enable. CPU không vào sleep, thuận tiện cho SWD attach với setup không có NRST.


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


Breakpoint hữu ích:

```gdb
break EXTI0_IRQHandler
continue
```

Sau khi hit, tiếp tục chạy để Button Service hoàn tất debounce:

```gdb
continue
```

Có thể đặt thêm breakpoint:

```gdb
break button_service_take_press
```


## 15. Test từng bước

1. Flash firmware khi nút chưa nhấn.
2. LED phải ở trạng thái OFF ban đầu.
3. Nhấn-thả một lần: LED đổi trạng thái đúng một lần.
4. Nhấn nhiều lần chậm: mỗi press toggle một lần.
5. Giữ nút: không tự lặp toggle vì chỉ falling edge khởi tạo press.
6. Thả nút: rising edge không được cấu hình nên không tạo press.
7. Bấm rất nhanh/bounce: debounce phải loại phần lớn chuyển đổi giả.

## 16. Troubleshooting

### Nhấn không có phản ứng

Kiểm tra:

- button thật sự nối PA0-GND,
- PA0 không bị peripheral khác chiếm,
- `EXTI0_IRQHandler` là strong symbol,
- `AFIO_EXTICR1` route về Port A,
- EXTI mask/FTSR đúng,
- NVIC IRQ 6 được enable.

### LED toggle nhiều lần một press

Kiểm tra:

- `BUTTON_SERVICE_DEBOUNCE_TIME_MS`,
- wiring dài/nhiễu,
- button contact bounce mạnh,
- application có gọi `button_service_take_press()` nhiều nơi không.

### EXTI ISR hit nhưng LED không đổi

Đặt breakpoint tại Button Service và xem PA0 sau 30 ms. Nếu pin đã HIGH, event bị loại đúng vì không còn là press ổn định.

## 17. Bài tập mở rộng

- tạo short/long press,
- thêm release event bằng rising edge,
- thêm double-click state machine,
- đổi button sang PBx/PCx và EXTI line tương ứng,
- publish button event qua Event Service queue,
- thêm nhiều button trên EXTI9_5 hoặc EXTI15_10.

## 18. Tài liệu liên quan

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
