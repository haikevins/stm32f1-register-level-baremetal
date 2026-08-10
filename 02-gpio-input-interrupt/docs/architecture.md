# Kiến trúc — 02-gpio-input-interrupt

## 1. Sơ đồ lớp

```text
                         +------------------+
PA0 ──> GPIO/EXTI MCAL ─> Board Button ───> Button Service
                                                 |
SysTick MCAL ─> Board Timebase ─> Time Service --+
                                                 |
                                                 v
                                            Application
                                                 |
                                                 v
                                      Indication Service
                                                 |
                                                 v
                                            Board LED
                                                 |
                                                 v
                                            MCAL GPIO
                                                 |
                                                 v
                                               PC13
```

## 2. Ownership

| State/resource | Owner |
|---|---|
| EXTI pending flags | MCAL EXTI |
| `g_exti_events` | MCAL EXTI |
| PA0 mapping | BSP Board Button |
| debounce timestamp/state | Button Service |
| LED toggle policy | Application |
| active-low PC13 | BSP Board LED |
| SysTick counter | MCAL SysTick |

## 3. Vì sao debounce không nằm trong ISR

Debounce cần chờ thời gian và đọc lại pin. Nếu thực hiện trong ISR sẽ:

- kéo dài interrupt latency,
- block interrupt khác,
- làm ISR phụ thuộc Time Service/BSP,
- phá dependency direction,
- khó kiểm thử và mở rộng.

Do đó ISR chỉ capture edge.

## 4. EXTI event lifecycle

```text
Hardware sets EXTI_PR.PR0
       ↓
EXTI0_IRQHandler
       ↓
record_pending_lines()
  ├─ read pending
  ├─ write 1 to clear
  └─ set software event bit
       ↓
thread mode
       ↓
mcal_exti_take_event()
  ├─ enter critical section
  ├─ test+clear bit
  └─ restore PRIMASK
       ↓
Button Service starts debounce
```

## 5. Concurrency model

Shared data duy nhất trực tiếp giữa ISR/thread trong EXTI path là `g_exti_events`. Critical section trong `mcal_exti_take_event()` bảo vệ thao tác read-modify-clear.

Debounce state chỉ thuộc thread mode nên không cần volatile/atomic.

## 6. Initialization order

EXTI được configure trong board init trước khi global IRQ enable. Button Service sau đó discard event có thể bị latch trong lúc init.

Điểm này tránh một "press giả" ngay khi application bắt đầu.

## 7. Layer boundaries

Application được phép:

```c
#include "button_service.h"
#include "indication_service.h"
```

Application không được:

```c
#include "mcal_exti.h"
#include "mcal_gpio.h"
#include "stm32f103xb.h"
```

## 8. Generic EXTI MCAL

`mcal_exti.c` hỗ trợ line 0..15 và chọn IRQ:

```text
0    → EXTI0_IRQn
1    → EXTI1_IRQn
2    → EXTI2_IRQn
3    → EXTI3_IRQn
4    → EXTI4_IRQn
5..9 → EXTI9_5_IRQn
10..15 → EXTI15_10_IRQn
```

Các grouped handler gọi cùng `record_pending_lines()` với mask phù hợp.

## 9. Failure propagation

Nếu GPIO/EXTI/timebase init fail:

```text
board_init() false
 → system_init() false
 → system_panic()
```

Không cố chạy Application với board resource chưa sẵn sàng.

## 10. Extension strategy

Muốn thêm button thứ hai:

- thêm mapping ở BSP,
- configure EXTI line tương ứng,
- Button Service giữ debounce state riêng,
- Application vẫn chỉ consume semantic press.

Không nên để Application biết line number.
