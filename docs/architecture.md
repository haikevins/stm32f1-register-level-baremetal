# Kiến trúc và luật dependency của template

Tài liệu này là quy ước kiến trúc cho project register-level. Mục tiêu không phải tạo thật nhiều layer, mà bảo đảm **hardware details đi xuống**, còn **product policy ở trên**.

## 1. Runtime layers

```text
Application
    ↓
Services
    ↓
BSP / ECUAL
    ↓
MCAL
    ↓
STM32F103 Device / Cortex-M3 Architecture
    ↓
Hardware
```

Ngoài runtime layers:

```text
system   = composition/lifecycle
startup  = reset/vector/runtime entry
linker   = memory placement
config   = compile-time policy
tools    = build/debug/static architecture checks
common   = portable primitives/types
```

## 2. Dependency matrix

| Source layer | Được phụ thuộc | Không được phụ thuộc |
|---|---|---|
| `app` | app, services, common, config | bsp, ecual, mcal, platform |
| `services` | services, bsp, ecual, common, config | app, raw platform |
| `ecual` | ecual, mcal, common, config | app, services |
| `bsp` | bsp, mcal, common, config | app, services |
| `mcal` | mcal, platform, common, config | bsp, ecual, services, app |
| `platform` | platform, common, config | mọi upper layer |
| `common` | common, config | hardware layer |
| `system` | composition exception | không nên chứa product behavior |
| `startup` | startup/common/config | runtime product layer |

`check_layers.py` encode gần đúng matrix này.

## 3. Application

Application trả lời câu hỏi:

```text
"Hệ thống nên làm gì?"
```

Ví dụ:

```text
nút press → toggle indication
ADC > threshold → LED on
UART byte → parse command
mỗi 100 ms → update UI
```

Application không trả lời:

```text
"GPIO register nào cần set?"
"USART1 ở địa chỉ nào?"
"PB6 là SCL?"
```

### Good

```c
if (button_service_take_press())
{
    indication_service_toggle(INDICATION_STATUS);
}
```

### Bad

```c
GPIOC->BSRR = ...;
```

## 4. Services

Service chuyển hardware/device capability thành semantic API.

Ví dụ:

```text
Time Service
Serial Service
Display Service
Memory Service
ADC Service
Indication Service
Button Service
```

Service có thể:

- aggregate data,
- debounce,
- filter,
- expose logical state,
- compose BSP/ECUAL.

Service không nên:

- hard-code pin,
- đọc raw STM32 register,
- chứa ISR peripheral.

## 5. BSP

BSP trả lời:

```text
"Trên board này resource nằm ở đâu?"
```

Ví dụ:

```text
status LED = PC13 active-low
UART console = USART1 PA9/PA10
OLED bus = I2C1 PB6/PB7
W25Q CS = PA4
```

BSP nên expose logical board resource:

```c
board_led_set(...)
board_uart_try_write_byte(...)
board_memory_bus_transfer(...)
```

Không đưa pin macro lên Service/Application.

## 6. ECUAL

ECUAL model external device, ví dụ:

```text
SSD1306
W25Q64
MPU sensor
EEPROM
```

Nó nên biết:

- command/register protocol của device,
- geometry/capability,
- sequencing requirement.

Nó không nên biết:

- product threshold,
- UI policy,
- Blue Pill pin cụ thể.

### Transport callback pattern

Vì BSP và ECUAL cùng tầng, layer checker không cho ECUAL gọi BSP trực tiếp trong design hiện tại. Service có thể compose:

```text
BSP bus callbacks
    ↓
ECUAL transport interface
```

Pattern này xuất hiện ở numbered examples.

## 7. MCAL

MCAL là owner của MCU peripheral behavior:

```text
RCC
GPIO
EXTI
USART
SPI
I2C
TIM
ADC
DMA
NVIC wrapper...
```

MCAL được phép:

- dereference register structs,
- set/clear bit,
- wait hardware flags với bounded loop,
- convert raw status thành portable error/event flags,
- giữ peripheral-owned static state,
- chứa ISR nếu module là owner thấp nhất.

MCAL không được gọi upward.

## 8. Platform Device

`platform/device/stm32f103xb` chứa:

```text
memory base addresses
register structs
IRQ numbers
register bit masks
compile-time offset checks nếu cần
```

Nó không chứa policy kiểu:

```text
UART console baud = 115200
OLED address = 0x3C
```

Đó là board/config concern.

## 9. Platform Architecture

`platform/arch/cortex-m3` chứa core-specific primitive:

```text
PRIMASK operations
WFI/NOP
barriers
SCB reset
SysTick/NVIC/SCB register layout
```

Nếu chuyển sang core khác, đây là boundary cần thay.

## 10. Common

`common/` chỉ chứa code portable:

- fixed data types/structs,
- generic static queue/ring algorithm nếu không hardware-specific,
- compiler abstraction,
- utility không include device header.

Nếu một common file cần `STM32_RCC`, nó không còn là common.

## 11. System as composition root

`system_init()` có thể gọi:

```text
board_init
service_init
application_init
```

Nó là nơi wiring dependency.

Không nên viết:

```text
nếu nút nhấn thì bật LED
```

trong `system/`.

## 12. Initialization order

Rule:

```text
low-level physical state
    ↓
board resources
    ↓
services
    ↓
application state
    ↓
enable global runtime
```

Template `main()` disable IRQ trước init và enable sau init.

Nếu peripheral IRQ được NVIC enable trong init, handler vẫn chưa chạy tới khi PRIMASK mở.

## 13. Interrupt ownership

Nguyên tắc:

> ISR thuộc tầng thấp nhất có ownership đủ để acknowledge peripheral và lưu dữ liệu/event cần thiết.

ISR có thể:

- clear pending,
- read/write data register,
- push byte vào fixed buffer,
- snapshot event/error,
- increment bounded counter.

ISR không:

- gọi Application callback,
- debounce bằng delay,
- parse protocol lớn,
- render OLED,
- erase flash,
- allocate,
- block chờ lâu.

## 14. Interrupt-to-thread handoff patterns

### Event bit

```text
ISR set bit
thread take+clear bit
```

Phù hợp event không cần đếm mọi occurrence.

### Counter

```text
ISR counter++
thread read/take
```

Phù hợp diagnostics.

### Ring buffer

```text
ISR producer
thread consumer
```

Phù hợp byte stream.

### Block-ready

```text
DMA ISR publish completed block
thread processing
```

Phù hợp ADC/DMA.

## 15. Critical sections

Critical section chỉ dùng quanh state thật sự shared.

Pattern:

```text
save PRIMASK
disable IRQ
small state update
restore PRIMASK nếu trước đó enabled
```

Không disable IRQ quanh:

- long copy nếu tránh được,
- erase flash,
- I/O transaction dài,
- delay.

Khi code hiện tại buộc copy block trong critical/ISR, document timing trade-off.

## 16. Volatile

`volatile` cần cho memory-mapped register và một số ISR-shared state để compiler không optimize access mất đi.

`volatile` **không** tự tạo:

- atomic multi-word transaction,
- mutex,
- memory ownership,
- race-free algorithm.

Thiết kế ownership vẫn là chính.

## 17. Polling API naming

Nếu API không chờ:

```c
try_read
try_write
take_event
take_error
```

Tên function phải phản ánh semantics.

Nếu API block:

- document timeout,
- document execution context được phép,
- tránh gọi từ ISR.

## 18. Error handling

Tầng thấp nên trả state dễ reason:

```text
bool
enum status
portable error flags
counters
```

Upper layer quyết định policy:

```text
retry
panic
degrade
show error
drop data
```

Không hard-code product recovery trong MCAL.

## 19. Clock ownership

RCC/MCAL biết clock tree; BSP biết peripheral nào dùng bus nào; Application không hard-code peripheral clock.

Driver nên nhận actual clock:

```c
mcal_x_init(peripheral_clock_hz, target_rate)
```

thay vì assume 72 MHz.

## 20. Board active level

Active-low/active-high là BSP concern.

Application:

```c
indication_service_set(..., true);
```

không nên:

```c
gpio_write(LOW); /* because LED active-low */
```

## 21. External-device geometry

W25Q page/sector geometry thuộc ECUAL.

Địa chỉ sector nào Application được phép erase là Application/product config concern.

Tương tự:

- sensor register map → ECUAL,
- alarm threshold → Service/Application.

## 22. Build-time configuration

Config nên có validation:

```c
#if VALUE == 0
#error ...
#endif
```

Dùng khi invalid config có thể phát hiện compile-time.

Không dùng magic literal rải nhiều tầng.

## 23. Dependency checker limitations

Checker dựa trên include path/header index. Nó không phát hiện mọi architecture violation, ví dụ:

- copy/paste raw address literal vào Application,
- function pointer callback upward mà không include trực tiếp,
- duplicated declaration,
- global extern không qua header.

Code review vẫn bắt buộc.

## 24. Architectural acceptance checklist

Trước merge module:

- [ ] Application không biết pin/peripheral.
- [ ] Service không truy cập register.
- [ ] BSP không gọi Application.
- [ ] ECUAL không chứa product behavior.
- [ ] MCAL không callback upward.
- [ ] ISR bounded và không block.
- [ ] Shared state có ownership rõ.
- [ ] Clock source/units rõ.
- [ ] Error semantics rõ.
- [ ] `make check-layers` pass.
- [ ] README/porting docs cập nhật.
