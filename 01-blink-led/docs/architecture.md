# Kiến trúc — 01-blink-led

## 1. Dependency graph

```text
Application
   ├──> Time Service ───────> Board Timebase ───> MCAL SysTick ───> Cortex-M3
   └──> Indication Service ─> Board LED ─────────> MCAL GPIO ──────> STM32F103

system/ = composition root
startup/linker/config/tools = infrastructure
```

## 2. Trách nhiệm từng tầng

| Tầng/module | Trách nhiệm |
|---|---|
| Application | Quyết định khi nào toggle logical LED |
| Time Service | Cung cấp thời gian monotonic theo ms và periodic helper |
| Indication Service | Biến logical indication thành lời gọi BSP |
| Event Service | Queue portable được init nhưng chưa tham gia blink behavior |
| Board | Khởi tạo clock, LED, timebase |
| Board LED | Map logical status LED tới PC13 active-low |
| Board Timebase | Map timebase tới SysTick |
| MCAL RCC | Cấu hình clock register |
| MCAL GPIO | Cấu hình/ghi/toggle GPIO |
| MCAL SysTick | Cấu hình core SysTick, sở hữu tick counter và ISR |
| Platform | Base address, register struct, bit mask |
| System | Thứ tự init, super-loop, panic/fault |
| Startup | Vector table, reset entry |
| Linker | Flash/RAM layout |

## 3. Initialization dependency

`board_init()` phải chạy trước Services vì:

- LED service cần GPIO đã configure,
- time service cần SysTick đã configure,
- Application lấy timestamp ngay trong `application_init()`.

Thứ tự:

```text
board_init
 → time_service_init
 → indication_service_init
 → event_service_init
 → application_init
```

## 4. Runtime data flow

### Time path

```text
SysTick hardware
 → SysTick_Handler
 → g_systick_ticks
 → mcal_systick_get_ticks
 → board_timebase_now_ms
 → time_service_now_ms
 → time_service_periodic_due
 → Application
```

### LED path

```text
Application
 → indication_service_toggle
 → board_led_toggle
 → mcal_gpio_toggle
 → GPIOC
```

## 5. Register ownership

Application/Services không biết `GPIOC`, `RCC`, `SysTick`.

- RCC register: MCAL RCC.
- GPIO register: MCAL GPIO.
- SysTick core register: MCAL SysTick.
- Pin PC13/active-low: BSP.

Đây là boundary quan trọng nhất của example.

## 6. ISR ownership

`SysTick_Handler` nằm trong `mcal_systick.c`.

ISR chỉ:

```text
g_systick_ticks++
```

Không toggle LED và không gọi Application/Service.

## 7. Concurrency

`g_systick_ticks` là `volatile uint32_t` được ghi trong ISR và đọc ở thread mode. Trên Cortex-M3, access aligned 32-bit là atomic cho use case này. Periodic helper dùng snapshot hiện tại và unsigned subtraction.

## 8. HSE fallback

Clock source là state của board/RCC, không phải concern của Application. Nếu HSE fail, timebase được init bằng core clock thực tế nên blink period theo milliseconds vẫn giữ đúng về logic.

## 9. Vì sao không busy delay trong Application

Busy delay sẽ:

- khóa super-loop,
- khó mở rộng nhiều task,
- tăng coupling giữa behavior và CPU clock.

Timestamp scheduling cho phép thêm task khác mà không thay đổi blink flow.

## 10. Event Service hiện tại

`event_service_init()` được gọi và queue storage được cấp tĩnh. Tuy nhiên blink Application không publish/consume event. Đây là scaffold có thể dùng khi mở rộng, không phải dependency bắt buộc của blink algorithm.

## 11. Failure path

Nếu `board_init()` hoặc `event_service_init()` fail:

```text
system_init() = false
 → system_panic()
 → global IRQ disabled
 → infinite NOP loop
```

Fault handlers cũng đi vào cùng panic path.

## 12. Extension boundaries

- Thêm pin → BSP + MCAL nếu mode mới.
- Thêm logical LED → Indication Service/BSP.
- Thêm periodic behavior → Application + Time Service.
- Thêm peripheral → MCAL + Platform.
- Không thêm register access vào Application chỉ vì example nhỏ.
