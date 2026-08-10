# Hướng dẫn thêm module mới

Tài liệu này mô tả quy trình thêm peripheral, board resource, external device, Service hoặc Application feature mà không phá architecture.

## 1. Bắt đầu từ requirement, không bắt đầu từ register

Trước khi tạo file, viết ngắn:

```text
Application cần capability gì?
API semantic nào hợp lý?
Hardware resource nào thực hiện capability?
Interrupt/polling/DMA?
Timing/error constraints?
```

Ví dụ requirement:

```text
Application cần gửi/nhận byte console non-blocking.
```

Không nên bắt đầu bằng:

```text
Tạo usart1.c rồi Application gọi register.
```

## 2. Quyết định module nằm tầng nào

| Câu hỏi | Tầng |
|---|---|
| Product quyết định gì? | Application |
| Semantic/domain behavior? | Service |
| Pin/resource của board? | BSP |
| Protocol external IC? | ECUAL |
| STM32 peripheral register? | MCAL |
| Address/bit/IRQ layout? | Platform |

Một feature thường cần nhiều module nhỏ ở nhiều tầng.

## 3. Thêm MCAL peripheral

Tạo:

```text
mcal/include/mcal_<peripheral>.h
mcal/src/mcal_<peripheral>.c
```

Ví dụ:

```text
mcal/include/mcal_spi.h
mcal/src/mcal_spi.c
```

### Public MCAL API nên nhận generic input

Ví dụ tốt:

```c
bool mcal_spi_init(instance,
                   peripheral_clock_hz,
                   max_bus_hz,
                   mode);
```

Thay vì:

```c
void init_spi1_for_w25q64(void);
```

Tên thứ hai trộn MCU peripheral với external device/product.

## 4. Thêm base address

Trong:

```text
platform/device/stm32f103xb/include/stm32f103xb_memory.h
```

thêm base theo device reference manual.

Ví dụ concept:

```c
#define STM32_SPI1_BASE (...)
```

Giữ address arithmetic ở device layer.

## 5. Thêm register struct

Trong device header:

```c
typedef struct
{
    volatile uint32_t CR1;
    ...
} stm32_spi_registers_t;
```

Cần:

- đúng register order,
- đúng offset/reserved slot,
- `volatile`,
- `const volatile` cho read-only register nếu design dùng.

Nếu module critical, thêm `_Static_assert(offsetof(...))`.

## 6. Thêm bit definitions

Trong:

```text
stm32f103xb_register_bits.h
```

chỉ thêm bit cần dùng.

Group theo register:

```c
/* SPI_CR1 */
#define ...
```

Tránh magic number trong MCAL.

## 7. Thêm RCC clock/reset support

Peripheral thường cần:

- AHB/APB clock enable,
- đôi khi reset bit,
- actual peripheral clock.

Xác định bus:

```text
APB1
APB2
AHB
```

Timer cần chú ý timer input clock có thể khác PCLK khi APB prescaler != 1.

## 8. Thêm GPIO alternate-function

Nếu peripheral có pin:

- mapping pin thuộc BSP,
- mode register call thuộc BSP qua MCAL GPIO,
- AFIO remap nếu cần phải có owner rõ.

Không hard-code PAx trong MCAL generic nếu module hỗ trợ nhiều instance/board.

## 9. Thiết kế polling

Nếu polling status flag:

```text
wait flag
```

phải có bound hoặc timeout nếu hardware có khả năng không trả trạng thái.

Tránh:

```c
while ((SR & FLAG) == 0U)
{
}
```

không giới hạn, trừ khi contract thật sự chấp nhận hard hang và đã document.

## 10. Thiết kế interrupt

Trước khi code ISR, xác định:

```text
peripheral IRQ source
pending clear sequence
data cần capture
shared state
consumer thread mode
overflow policy
```

ISR nên nhỏ.

### Event bit

Dùng cho edge semantic.

### Ring buffer

Dùng cho stream.

### Block event

Dùng cho DMA.

## 11. NVIC

Nếu thêm IRQ:

- IRQ number vào device header nếu chưa có,
- priority range,
- clear pending trước enable nếu cần,
- enable source peripheral trước/sau NVIC theo sequence an toàn,
- global IRQ chỉ mở sau init theo template lifecycle.

## 12. Strong handler name

Tên phải khớp vector table:

```c
void USART1_IRQHandler(void)
```

Sau build kiểm tra:

```bash
arm-none-eabi-nm build/firmware.elf | grep USART1_IRQHandler
```

Interrupt được implement phải là strong symbol, không còn weak alias.

## 13. Thêm Board resource

Tạo:

```text
bsp/bluepill/include/board_<resource>.h
bsp/bluepill/src/board_<resource>.c
```

Ví dụ:

```text
board_uart
board_led
board_memory_bus
```

BSP chịu trách nhiệm:

- pin,
- polarity,
- peripheral instance,
- board clock resource,
- chip select,
- wiring-specific behavior.

## 14. `board_pins.h`

Dùng macro semantic:

```c
BOARD_UART_TX_PIN
BOARD_MEMORY_CS_PIN
BOARD_STATUS_LED_PIN
```

Không dùng generic:

```c
PIN1
PIN2
```

vì mất meaning.

## 15. Thêm external device ECUAL

Tạo:

```text
ecual/include/<device>.h
ecual/src/<device>.c
```

Driver chứa:

- command/register map,
- geometry,
- protocol sequence,
- validation.

Không chứa:

- product threshold,
- UI logic,
- board pin.

## 16. Transport callback cho ECUAL

Nếu layer rule không cho ECUAL include BSP, tạo transport interface:

```c
typedef struct
{
    bool (*transfer)(...);
    void (*select)(void);
    void (*deselect)(void);
} device_transport_t;
```

Service compose BSP callbacks rồi truyền vào ECUAL init.

Điều này giúp unit test ECUAL sau này dễ hơn.

## 17. Thêm Service

Tạo:

```text
services/include/<name>_service.h
services/src/<name>_service.c
```

Service API nên dùng unit semantic:

```text
millivolts
permille
bytes
milliseconds
logical indication
```

thay vì raw register values nếu không cần.

## 18. Service processing pattern

Nếu data từ ISR/DMA:

```c
void service_process(void);
bool service_take_result(...);
```

`service_process()` chạy thread mode và thực hiện computation nặng.

Application gọi trong super-loop.

## 19. Thêm Application behavior

Application chỉ include Services/Common/Config.

State machine phải:

- non-blocking khi practical,
- explicit state,
- bounded work mỗi iteration,
- không delay busy dài,
- không truy cập register.

## 20. Update `system_init()`

Thứ tự chuẩn:

```text
board init
↓
service init
↓
external device/service init phụ thuộc board
↓
application init
```

Chỉ composition root biết toàn bộ dependency.

## 21. Global IRQ lifecycle

Template:

```text
IRQ disabled
system_init
IRQ enabled
super-loop
```

Nếu init function cần interrupt để tiến triển, có ba lựa chọn:

1. đổi init design để không cần IRQ,
2. enable IRQ có kiểm soát trước phần đó và document lifecycle,
3. chuyển operation ra runtime process.

Không âm thầm dùng SysTick timeout khi global IRQ đang disabled.

## 22. Add configuration

Chia config theo concern:

```text
board_config.h
mcal_config.h
service_config.h
application_config.h
```

Template chỉ có `project_config.h`, nhưng numbered examples cho thấy pattern chia config khi project lớn hơn.

## 23. Compile-time validation

Ví dụ ring size:

```c
#if SIZE < 2
#error ...
#endif

#if (SIZE & (SIZE - 1)) != 0
#error ...
#endif
```

Validation gần source of truth.

## 24. Không dùng heap

Ưu tiên:

```text
static storage
fixed ring
fixed queue
caller-owned buffer
```

Nếu một feature thật sự cần allocator, đó là architectural decision riêng chứ không thêm ngầm.

## 25. Add debug observability

Cho learning/example project, expose bounded diagnostics:

```text
error counters
last value
sequence number
overflow count
state enum
```

Không biến mọi internal variable thành global.

## 26. Update documentation

Mỗi feature/example nên có:

### README

- purpose,
- wiring,
- configuration,
- behavior,
- build/flash/debug,
- expected result,
- troubleshooting.

### architecture.md

- dependency graph,
- ownership,
- ISR/thread handoff,
- concurrency,
- state.

### porting_guide.md

- pin/peripheral/clock changes,
- MCU changes,
- validation checklist.

## 27. Run layer checker

```bash
make check-layers
```

Sửa architecture violation thay vì thêm exception không cần thiết.

## 28. Build sạch

```bash
make clean
make
make size
```

Đọc warnings; không chỉ nhìn exit code.

## 29. Inspect map/symbols

Kiểm tra:

```bash
arm-none-eabi-size -A build/firmware.elf
arm-none-eabi-nm build/firmware.elf
```

Với IRQ, xác nhận symbol mạnh.

Với large static buffer, xem `.bss` tăng đúng dự kiến.

## 30. Hardware bring-up strategy

Không test toàn stack cùng lúc nếu peripheral phức tạp.

Ví dụ SPI flash:

1. xác nhận SCK/CS,
2. đọc JEDEC ID,
3. đọc status,
4. WREN/WEL,
5. erase,
6. program,
7. read-back.

ADC/DMA:

1. timer trigger,
2. ADC conversion,
3. DMA movement,
4. IRQ,
5. Service processing.

## 31. Checklist thêm module

- [ ] Requirement/API semantic rõ.
- [ ] Layer owner đúng.
- [ ] Platform address/register bits đúng.
- [ ] Clock source đúng.
- [ ] GPIO mode đúng.
- [ ] Error/status clear đúng.
- [ ] Polling có bound.
- [ ] ISR bounded.
- [ ] Shared state có ownership.
- [ ] Không callback upward từ ISR.
- [ ] Init order đúng.
- [ ] Layer checker pass.
- [ ] Clean build.
- [ ] Debug observability có đủ.
- [ ] Docs cập nhật.
