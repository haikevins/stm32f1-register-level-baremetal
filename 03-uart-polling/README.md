# 03-uart-polling — USART1 polling với API non-blocking

Example này cấu hình **USART1** trên STM32F103C8T6 để truyền/nhận dữ liệu ở **115200 baud, 8N1** bằng polling. Firmware gửi greeting sau reset rồi echo từng byte nhận được. Điểm quan trọng là API UART không busy-wait: mỗi lời gọi chỉ kiểm tra trạng thái peripheral và trả về ngay.

## 1. Mục tiêu học tập

Example minh họa:

- cấu hình GPIO alternate-function cho UART TX/RX,
- bật clock USART1 trên APB2,
- tính `BRR` từ peripheral clock và baud rate,
- enable TE/RE/UE,
- đọc `SR`/`DR` đúng sequence để nhận byte và clear receive/error flags,
- polling `RXNE` và `TXE`,
- thiết kế `try_read`/`try_write` không blocking,
- xử lý parity/framing/noise/overrun ở MCAL,
- giữ Application độc lập với register/peripheral instance,
- phân biệt polling architecture với interrupt-buffered architecture ở Example 04.

## 2. Wiring

Dùng USB-to-UART **logic 3.3 V**:

```text
Blue Pill              USB-UART
--------------------------------
PA9 / USART1_TX  ----> RX
PA10 / USART1_RX <---- TX
GND              ----- GND
```

Không nối TX 5 V của adapter trực tiếp vào PA10.

Terminal:

```text
Baud:        115200
Data bits:   8
Parity:      none
Stop bits:   1
Flow control:none
```

## 3. Expected behavior

Sau reset terminal nhận:

```text
STM32F103 UART polling ready
```

Sau đó ký tự gửi từ terminal được echo lại.

Ví dụ:

```text
TX host:  H e l l o
RX host:  H e l l o
```

## 4. Compile-time configuration

`config/board_config.h`:

```c
#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TARGET_CLOCK_HZ   (72000000UL)
#define BOARD_UART_BAUD_RATE    (115200UL)
```

Pin mapping:

```c
#define BOARD_UART_TX_PORT MCAL_GPIO_PORT_A
#define BOARD_UART_TX_PIN  (9U)

#define BOARD_UART_RX_PORT MCAL_GPIO_PORT_A
#define BOARD_UART_RX_PIN  (10U)
```

Greeting:

```c
#define APPLICATION_UART_GREETING \
    "STM32F103 UART polling ready\r\n"
```

## 5. Initialization flow

```text
system_init()
  ├─ board_init()
  │   ├─ configure system clock
  │   └─ board_uart_init(PCLK2)
  │       ├─ PA9  AF push-pull
  │       ├─ PA10 floating input
  │       └─ mcal_usart_init(USART1, PCLK2, 115200)
  ├─ serial_service_init()
  └─ application_init()
```

Không có USART IRQ/NVIC configuration.

## 6. GPIO configuration

### TX — PA9

TX là peripheral output:

```text
PA9 = Alternate Function Push-Pull, 50 MHz mode
```

### RX — PA10

RX là input:

```text
PA10 = Input Floating
```

Application không thấy pin mapping này; BSP sở hữu pin.

## 7. Baud-rate register

MCAL tính:

```text
baud_divider ≈ peripheral_clock_hz / baud_rate
```

với rounding:

```c
(peripheral_clock_hz + baud_rate / 2) / baud_rate
```

Ở PCLK2 = 72 MHz:

```text
72,000,000 / 115,200 = 625
BRR = 625 = 0x0271
```

Trong STM32F1 oversampling-by-16 mặc định, giá trị này encode mantissa/fraction của USARTDIV.

Nếu clock fallback HSI 8 MHz, BSP truyền PCLK2 thực tế để BRR được tính lại.

## 8. USART setup

MCAL thực hiện:

```text
RCC_APB2ENR.USART1EN = 1
USART1_CR1 = 0
USART1_CR2 = 0
USART1_CR3 = 0
USART1_BRR = divider
USART1_CR1 = TE | RE | UE
```

Reset defaults giữ:

```text
8 data bits
no parity
1 stop bit
```

## 9. Polling receive path

`mcal_usart_try_read_byte()`:

1. đọc `USARTx->SR`,
2. snapshot hardware error bits,
3. nếu có error hoặc `RXNE`, đọc `DR`,
4. đọc SR rồi DR clear `RXNE` và PE/FE/NE/ORE theo STM32F1 behavior,
5. nếu có error: accumulate portable error flags, trả `false`,
6. nếu hợp lệ: trả byte và `true`.

API upper layer:

```c
bool serial_service_try_read_byte(uint8_t *byte);
```

`false` không nhất thiết là lỗi; thường chỉ có nghĩa là chưa có byte.

## 10. Polling transmit path

`mcal_usart_try_write_byte()`:

```text
TXE = 0 → return false
TXE = 1 → write DR, return true
```

Không có vòng chờ `while (TXE == 0)` trong API.

## 11. Application state machine

Application giữ:

```text
g_greeting_index
g_echo_pending
g_echo_byte
```

Priority trong mỗi loop iteration:

```text
1. collect error flags
2. nếu greeting chưa gửi hết:
     try gửi 1 byte greeting
     return
3. nếu có echo byte pending:
     try gửi byte đó
     return
4. try nhận 1 byte
5. nếu nhận được:
     lưu byte thành pending echo
```

Điều này bảo đảm mỗi call ngắn, không blocking.

## 12. Vì sao phải giữ `g_echo_pending`

Có thể nhận byte trong khi TX chưa sẵn sàng. Nếu Application đọc RX rồi cố gửi ngay và bỏ byte khi `try_write` false thì dữ liệu sẽ mất. Pending slot giữ một byte cho tới khi TXE sẵn sàng.

Đây chưa phải ring buffer; Example 04 giải quyết throughput tốt hơn bằng interrupt + buffer.

## 13. Error mapping

MCAL chuyển hardware flag thành portable flags:

```text
bit 0 → parity error
bit 1 → framing error
bit 2 → noise error
bit 3 → overrun error
```

Application expose:

```c
application_uart_error_events
application_uart_error_flags
```

`error_events` tăng mỗi lần Service trả một nhóm error mới; `error_flags` OR tích lũy các loại lỗi từng xuất hiện.

## 14. Debug symbols

```gdb
p/x application_uart_last_rx_byte
p application_uart_rx_count
p application_uart_tx_count
p application_uart_error_events
p/x application_uart_error_flags
```

Interpretation:

- `rx_count`: số byte Application nhận hợp lệ,
- `tx_count`: số byte đã được ghi thành công vào USART data path,
- `last_rx_byte`: byte hợp lệ cuối,
- `error_flags`: loại error đã quan sát.

## 15. Interrupt policy

`USART1_IRQHandler` không được implement ở example này, nên vector startup giữ weak default handler.

Đây là intentional: data path là polling hoàn toàn.

## 16. Idle behavior

`system_idle()` dùng `NOP`. Điều này phù hợp cả hai mục tiêu:

- polling loop tiếp tục kiểm tra USART,
- SWD attach dễ dự đoán với probe không có NRST.

Nếu đổi sang `WFI` mà không có interrupt để wake cho UART polling, RX processing sẽ không hoạt động theo mong đợi.

## 17. Kiến trúc

```text
Application
    ↓
Serial Service
    ↓
Board UART
    ↓
MCAL USART + GPIO + RCC
    ↓
USART1 / GPIOA / RCC registers
```

Application không biết:

- USART1,
- PA9/PA10,
- APB2,
- BRR,
- RXNE/TXE.


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


Debug có thể đặt breakpoint:

```gdb
break mcal_usart_try_read_byte
break mcal_usart_try_write_byte
```

Không đặt breakpoint liên tục trong polling loop nếu terminal throughput là điều cần đo.


## 18. Test procedure

1. Nối USB-UART đúng TX↔RX.
2. Mở terminal 115200 8N1.
3. Flash/reset board.
4. Xác nhận greeting.
5. Gửi chuỗi ASCII.
6. Xác nhận echo đúng byte.
7. Gửi liên tục nhanh để quan sát giới hạn polling.
8. Inspect counters bằng GDB.

## 19. Troubleshooting

### Không thấy greeting

Kiểm tra:

- PA9 → RX adapter,
- common GND,
- terminal đúng baud,
- adapter logic 3.3 V,
- PCLK2/BRR,
- USART1 clock enable,
- PA9 AF push-pull.

### Greeting thành ký tự rác

Ưu tiên kiểm tra:

- baud mismatch,
- clock HSE/PLL thực tế,
- BRR,
- terminal parity/stop bits.

### Gõ nhưng không echo

Kiểm tra:

- adapter TX → PA10,
- PA10 input floating,
- `RXNE`,
- error flags,
- host local echo có đang gây nhầm không.

### Mất byte khi gửi nhanh

Đây là giới hạn tự nhiên của polling + một pending echo byte. Dùng Example 04 để học ring buffer/interrupt.

## 20. Tài liệu liên quan

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
