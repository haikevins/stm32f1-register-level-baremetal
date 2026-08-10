# 04-uart-interrupt-ring-buffer — USART1 interrupt + RX/TX ring buffer

Example này nâng cấp `03-uart-polling` bằng cách chuyển data movement sang **USART1 interrupt** và hai ring buffer tĩnh. RXNE interrupt đưa byte nhận vào RX ring; TXE interrupt lấy byte từ TX ring để truyền. Application vẫn chỉ làm greeting/echo ở thread mode và không gọi trực tiếp ISR/peripheral.

## 1. Mục tiêu học tập

Example minh họa:

- USART1 RXNE/TXE interrupt,
- enable/priority NVIC register-level,
- ring buffer single-producer/single-consumer,
- power-of-two capacity và mask wrap,
- RX overflow handling,
- hardware receive error capture,
- TX interrupt start/stop,
- critical section ngắn để tránh race khi enable TXEIE,
- phân tách ISR data movement khỏi Application policy,
- non-blocking Service API với throughput tốt hơn polling.

## 2. Wiring

Giống Example 03:

```text
Blue Pill              USB-UART 3.3 V
--------------------------------------
PA9 / USART1_TX  ----> RX
PA10 / USART1_RX <---- TX
GND              ----- GND
```

Terminal:

```text
115200 baud
8 data bits
no parity
1 stop bit
no flow control
```

Sau reset:

```text
STM32F103 UART interrupt ring buffer ready
```

Mọi byte nhập được echo lại.

## 3. Compile-time configuration

`config/mcal_config.h`:

```c
#define MCAL_USART_RX_BUFFER_SIZE (128UL)
#define MCAL_USART_TX_BUFFER_SIZE (128UL)
#define MCAL_USART1_IRQ_PRIORITY  (2UL)
```

Compile-time guards yêu cầu:

- mỗi buffer >= 2,
- size là power of two,
- size <= 32768 để index 16-bit an toàn,
- priority 0..15.

Với implementation giữ một slot trống để phân biệt full/empty:

```text
storage = 128 byte
usable  = 127 byte
```

## 4. Ring-buffer model

### RX ring

```text
Producer: USART1_IRQHandler
Consumer: thread mode / Application
```

ISR ghi `rx_head`; thread mode đọc/tăng `rx_tail`.

### TX ring

```text
Producer: thread mode / Application
Consumer: USART1_IRQHandler
```

Thread mode ghi `tx_head`; ISR đọc/tăng `tx_tail`.

Mỗi index chỉ có một writer chính, giúp concurrency đơn giản.

## 5. Empty và full

Empty:

```text
head == tail
```

Full:

```text
next(head) == tail
```

Power-of-two size cho phép wrap bằng mask:

```text
next = (index + 1) & (SIZE - 1)
```

Không cần modulo division runtime.

## 6. Initialization flow

```text
system_init()
  ├─ board_init()
  │   ├─ clock setup
  │   └─ board_uart_init(PCLK2)
  │       ├─ GPIO PA9/PA10
  │       └─ mcal_usart_init()
  │           ├─ reset ring/error state
  │           ├─ USART setup
  │           ├─ RXNEIE enable
  │           └─ NVIC USART1 enable
  ├─ serial_service_init()
  └─ application_init()
```

Global IRQ chỉ enable sau `system_init()`.

## 7. RX interrupt path

Khi `RXNE` hoặc error:

```text
USART1 hardware
    ↓
USART1_IRQHandler
    ↓
mcal_usart_handle_interrupt
    ↓
handle_rx_interrupt
    ├─ snapshot SR
    ├─ read DR
    ├─ convert PE/FE/NE/ORE nếu có
    └─ nếu byte hợp lệ:
         next_head = next(rx_head)
         nếu full:
             rx_overflow_count++
         else:
             rx_buffer[head] = byte
             rx_head = next_head
```

Đọc SR rồi DR đồng thời thực hiện sequence clear các receive/error flags liên quan của STM32F1.

## 8. TX interrupt path

Khi `TXEIE` bật và `TXE` set:

```text
USART1_IRQHandler
  ↓
handle_tx_interrupt
  ├─ tx_tail != tx_head?
  │    ├─ yes → DR = tx_buffer[tail]
  │    │        advance tail
  │    └─ no  → disable TXEIE
```

TXE interrupt chỉ tồn tại khi có khả năng còn data để gửi; khi ring empty, ISR tắt TXEIE để tránh interrupt storm.

## 9. Thread-mode write và race TX start

`mcal_usart_try_write_byte()` cần xử lý race:

```text
Thread enqueue byte
        ↕
ISR có thể thấy queue empty và tắt TXEIE
```

Implementation dùng critical section ngắn:

1. disable global IRQ, lưu PRIMASK,
2. kiểm tra TX ring full,
3. ghi byte + cập nhật head,
4. set `TXEIE`,
5. restore PRIMASK trước đó.

Điều này bảo đảm byte mới enqueue luôn có interrupt để drain.

Critical section không bao quanh toàn transaction UART; chỉ bao quanh state update nhỏ.

## 10. Thread-mode read

`try_read_byte()` kiểm tra RX ring:

```text
tail == head → false
else:
    byte = buffer[tail]
    tail = next(tail)
    return true
```

Không block.

## 11. RX overflow semantics

Nếu ISR nhận byte khi RX ring full:

- byte mới bị drop,
- `rx_overflow_count` tăng.

Upper layer gọi:

```c
serial_service_take_rx_overflow_count();
```

Application cộng vào:

```c
application_uart_rx_overflow_count
```

Điều này giúp phân biệt:

- hardware ORE: USART không được phục vụ kịp,
- software ring overflow: ISR nhận được byte nhưng ring upper layer chưa drain kịp.

## 12. Hardware error flags

Portable mapping:

```text
bit 0 → parity
bit 1 → framing
bit 2 → noise
bit 3 → overrun
```

Error state được tích lũy ở MCAL và upper layer dùng take-and-clear API.

## 13. Application behavior

Application vẫn có state nhỏ:

```text
g_greeting_index
g_echo_pending
g_echo_byte
```

Greeting được enqueue từng byte. Sau greeting, Application lấy byte từ RX ring và giữ một pending echo nếu TX ring tạm thời full.

Dù ring TX thường có chỗ, pending byte giữ contract không mất dữ liệu tại boundary Application.

## 14. Debug symbols

```gdb
p/x application_uart_last_rx_byte
p application_uart_rx_count
p application_uart_tx_count
p application_uart_error_events
p/x application_uart_error_flags
p application_uart_rx_overflow_count
```

Bình thường khi terminal gửi với tốc độ hợp lý:

```text
error_flags = 0
rx_overflow_count = 0
```

## 15. Interrupt ownership

`USART1_IRQHandler` là strong symbol trong `mcal_usart.c`.

ISR không:

- echo trực tiếp,
- gọi Serial Service,
- gọi Application,
- parse command,
- delay,
- allocate.

## 16. Kiến trúc

```text
Application
    ↓
Serial Service
    ↓
Board UART
    ↓
MCAL USART
    ├─ RX/TX rings
    ├─ error/overflow state
    └─ USART1_IRQHandler
    ↓
USART1 + NVIC registers
```

Pin mapping và clock vẫn thuộc BSP/RCC.

## 17. So sánh với Example 03

| Hạng mục | 03 polling | 04 IRQ + ring |
|---|---|---|
| RX | Application poll RXNE | ISR → RX ring |
| TX | Application poll TXE | TX ring → TXE ISR |
| Buffer | 1 pending echo | 127 usable RX + 127 usable TX |
| NVIC | Không | USART1 IRQ |
| Throughput tolerance | Thấp hơn | Cao hơn |
| Concurrency | Ít | Ring/ISR/thread |
| Learning focus | Peripheral basics | Concurrency + buffering |

## 18. Idle behavior

Dù UART đã interrupt-driven, example vẫn dùng `NOP` thay `WFI` để giữ debug behavior nhất quán với ST-Link không NRST.


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


Breakpoint ISR:

```gdb
break USART1_IRQHandler
continue
```

Inspect counters sau khi chạy:

```gdb
p application_uart_rx_count
p application_uart_tx_count
p application_uart_rx_overflow_count
p/x application_uart_error_flags
```


## 19. Stress test

1. Gửi chuỗi ngắn → echo đúng.
2. Paste vài trăm ký tự → kiểm tra echo/counters.
3. Halt CPU lâu trong GDB trong khi host tiếp tục gửi → có thể tạo hardware ORE hoặc overflow; đây là behavior mong đợi khi debugger ngăn firmware phục vụ data.
4. Resume và inspect counters.
5. Không dùng breakpoint ISR khi đo throughput vì breakpoint phá timing.

## 20. Troubleshooting

### Không có greeting

Kiểm tra như Example 03 cộng thêm:

- NVIC USART1 enable,
- RXNEIE,
- TXEIE được set sau enqueue greeting,
- `USART1_IRQHandler` không còn weak.

### Greeting chỉ có byte đầu

Khả năng TXEIE bị tắt và không được re-enable sau enqueue. Kiểm tra critical-section path trong `try_write`.

### RX overflow tăng

Application không drain đủ nhanh hoặc debugger halt quá lâu. Tăng ring chỉ che latency chứ không sửa blocking task.

### Hardware overrun tăng

ISR không được chạy đủ nhanh: priority, global IRQ, long critical section hoặc debugger halt.

## 21. Tài liệu liên quan

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
