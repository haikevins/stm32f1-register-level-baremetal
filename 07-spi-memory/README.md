# 07-spi-memory — SPI1 + W25Q64 NOR Flash

Example này điều khiển **W25Q64 64-Mbit (8 MiB)** qua **SPI1** bằng register-level polling. Firmware đọc JEDEC ID, xóa sector cuối 4 KiB, program 32 byte test pattern, đọc lại và verify từng byte. Khi test pass, LED PC13 toggle mỗi 500 ms; nếu erase/program/verify fail sau khi memory init đã thành công, LED giữ sáng.

> **Cảnh báo:** đây là demo destructive. Sector cuối `0x007FF000..0x007FFFFF` bị erase mỗi lần reset.

## 1. Mục tiêu học tập

Example minh họa:

- SPI master full-duplex,
- GPIO AF cho SCK/MOSI và floating input cho MISO,
- software-controlled chip select,
- chọn baud prescaler theo PCLK2 và max SPI clock,
- polling `TXE`, `RXNE`, `BSY`,
- giao thức command/response của SPI NOR,
- JEDEC ID,
- Status Register-1 `BUSY`/`WEL`,
- Write Enable trước program/erase,
- 24-bit address,
- 256-byte page boundary,
- 4 KiB sector erase,
- bounded busy polling,
- ECUAL W25Q64 với transport callbacks,
- destructive self-test và GDB diagnostics.

## 2. Wiring

```text
STM32F103C8T6       W25Q64
--------------------------------
3.3V        ------  VCC
GND         ------  GND
PA4         ------  CS
PA5         ------  CLK
PA6         ------  D1 / DO / MISO
PA7         ------  D0 / DI / MOSI
```

Mapping quan trọng:

```text
D1 = DO = MISO = PA6
D0 = DI = MOSI = PA7
```

Dùng 3.3 V logic và common ground.

## 3. Expected behavior

Startup test:

```text
Read JEDEC ID
    ↓
verify manufacturer/capacity
    ↓
erase sector 0x007FF000
    ↓
wait BUSY=0
    ↓
page program 32 bytes
    ↓
wait BUSY=0
    ↓
read 32 bytes
    ↓
byte-by-byte verify
```

Pass:

```text
PC13 toggles every 500 ms
```

Runtime test failure:

```text
PC13 steady ON
```

Nếu JEDEC init fail, `system_init()` fail và firmware đi vào `system_panic()` trước Application test.

## 4. W25Q64 geometry trong driver

```c
#define W25Q64_SIZE_BYTES        (8388608UL)
#define W25Q64_PAGE_SIZE_BYTES   (256U)
#define W25Q64_SECTOR_SIZE_BYTES (4096UL)
```

Address range:

```text
0x000000 .. 0x7FFFFF
```

Test sector:

```c
#define MEMORY_DEMO_TEST_SECTOR_ADDRESS (0x007FF000UL)
```

Test length:

```c
#define MEMORY_DEMO_TEST_LENGTH (32U)
```

## 5. JEDEC ID

Driver command:

```text
0x9F = JEDEC ID
```

Đọc 3 byte:

```text
manufacturer_id
memory_type
capacity_id
```

Driver yêu cầu:

```text
manufacturer = 0xEF
capacity     = 0x17
```

`memory_type` được lưu lại nhưng không ép bằng một giá trị duy nhất, giúp example linh hoạt hơn giữa revision thuộc W25Q64 class.

Với module đã test phổ biến:

```text
EF 40 17
```

## 6. SPI configuration

`config/board_config.h`:

```c
#define BOARD_MEMORY_SPI_MAX_HZ (5000000UL)
```

SPI:

```text
Peripheral: SPI1
Mode:       0
CPOL:       0
CPHA:       0
Frame:      8 bit
Bit order:  MSB first
NSS:        software
CS:         PA4 GPIO
```

MCAL set:

```text
CR1.MSTR = 1
CR1.SSM  = 1
CR1.SSI  = 1
CR1.BR   = selected prescaler
CR1.SPE  = 1
```

Mode 0 giữ `CPOL=0`, `CPHA=0`.

## 7. SPI clock selection

MCAL duyệt divisor:

```text
/2, /4, /8, /16, /32, /64, /128, /256
```

và chọn divisor nhỏ nhất sao cho rounded-up SPI clock không vượt `BOARD_MEMORY_SPI_MAX_HZ`.

Với PCLK2 72 MHz:

```text
/8  = 9 MHz   > 5 MHz
/16 = 4.5 MHz <= 5 MHz
```

Nên SPI chạy 4.5 MHz.

HSI fallback:

```text
PCLK2 = 8 MHz
/2 = 4 MHz <= 5 MHz
```

## 8. Chip select

CS là GPIO PA4, không dùng hardware NSS.

Transaction:

```text
CS LOW
  ↓
command/header/data
  ↓
wait SPI BSY=0
  ↓
CS HIGH
```

BSP preload CS HIGH trước/đồng thời cấu hình output để tránh chọn flash ngoài ý muốn trong init.

## 9. SPI transfer primitive

`mcal_spi_transfer()` hỗ trợ:

```c
tx != NULL, rx == NULL  → transmit, discard received byte
tx == NULL, rx != NULL  → transmit 0xFF dummy, collect RX
tx != NULL, rx != NULL  → full-duplex exchange
```

Mỗi byte:

```text
wait TXE
DR = tx/dummy
wait RXNE
rx = DR
```

Cuối transfer:

```text
wait BSY = 0
```

Polling có `MCAL_SPI_POLL_TIMEOUT_CYCLES` để tránh treo vĩnh viễn nếu peripheral không đạt trạng thái yêu cầu.

## 10. W25Q64 command set dùng trong demo

```text
0x9F  JEDEC ID
0x05  Read Status Register-1
0x06  Write Enable
0x03  Read Data
0x02  Page Program
0x20  4 KiB Sector Erase
```

Không dùng Quad SPI; chỉ standard SPI 1-1-1.

## 11. Status Register-1

Driver dùng:

```text
bit 0 BUSY
bit 1 WEL
```

### BUSY

Sau program/erase, flash tự thực hiện internal operation. SPI command đã gửi xong không có nghĩa dữ liệu đã hoàn tất. Driver poll `0x05` tới khi:

```text
BUSY = 0
```

### WEL

Trước operation thay đổi flash:

```text
send 0x06 Write Enable
read Status-1
verify WEL = 1
```

Nếu WEL không set, operation không được tiếp tục.

## 12. Bounded polling

`config/service_config.h`:

```c
#define W25Q64_READY_POLL_LIMIT        (10000UL)
#define W25Q64_PAGE_PROGRAM_POLL_LIMIT (100000UL)
#define W25Q64_SECTOR_ERASE_POLL_LIMIT (1000000UL)
```

Đây là **số lần poll**, không phải millisecond timeout.

Lý do: erase/program test chạy trong `application_init()` khi `main()` vẫn giữ global IRQ disabled. Không thể dựa vào SysTick tick để đo timeout ở thời điểm này.

## 13. Page Program rules

`w25q64_page_program()` validate:

- `data != NULL`,
- `length > 0`,
- `length <= 256`,
- address range hợp lệ,
- request không vượt page boundary.

Page offset:

```text
address % 256
```

Condition:

```text
page_offset + length <= 256
```

Điều này tránh page-wrap behavior không mong muốn.

## 14. Sector erase rules

`w25q64_sector_erase(address)` align xuống sector:

```text
address -= address % 4096
```

Sau đó:

```text
wait ready
write enable
0x20 + A23..A0
CS high
poll BUSY
```

## 15. Read transaction

```text
CS low
0x03
A23 A15 A7
dummy clocks while receiving data
CS high
```

Trong implementation:

```text
header[4] = command + 3 address bytes
transfer(header)
transfer(NULL, data, length)  // sends 0xFF dummy
```

## 16. ECUAL transport

`w25q64_transport_t`:

```c
transfer(...)
select()
deselect()
```

Memory Service tạo struct callback từ Board Memory Bus và truyền vào `w25q64_init()`.

Do đó W25Q64 driver không include:

- `mcal_spi.h`,
- `board_pins.h`,
- `stm32f103xb.h`.

## 17. Application self-test

Pattern 32 byte bắt đầu:

```text
0x53 = 'S'
```

và byte cuối:

```text
0xA5
```

Flow:

```text
memory_service_get_jedec_id()
erase sector
program pattern
read back
verify byte-by-byte
```

Nếu mismatch, Application lưu index đầu tiên.

## 18. Debug globals

```gdb
p/x application_memory_manufacturer_id
p/x application_memory_type_id
p/x application_memory_capacity_id

p/x application_memory_test_address
p application_memory_test_sequence
p application_memory_error_count

p application_memory_erase_ok
p application_memory_program_ok
p application_memory_verify_ok
p application_memory_test_passed

p/x application_memory_first_mismatch_index
p/x application_memory_readback_first_byte
p/x application_memory_readback_last_byte
```

Pass điển hình:

```text
manufacturer_id = 0xEF
type_id         = 0x40
capacity_id     = 0x17

test_address    = 0x007FF000
erase_ok        = true
program_ok      = true
verify_ok       = true
test_passed     = true
error_count     = 0

readback_first  = 0x53
readback_last   = 0xA5
first_mismatch  = 0xFF
```

## 19. LED indication

PC13 active-low:

- test pass → toggle mỗi 500 ms,
- Application test failure → steady ON.

Heartbeat dùng Time Service sau khi global IRQ đã enable và super-loop chạy.

## 20. Interrupt policy

SPI là polling:

```text
SPI1_IRQHandler = weak/default
```

SysTick strong cho heartbeat:

```text
SysTick_Handler = strong
```

Không có SPI ISR.

## 21. Kiến trúc

```text
Application
   ├─> Memory Service
   │      ├─> W25Q64 ECUAL
   │      └─> Board Memory Bus
   │              ├─> MCAL SPI
   │              └─> MCAL GPIO
   │
   ├─> Indication Service → Board LED → MCAL GPIO
   └─> Time Service → Board Timebase → MCAL SysTick
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


Test JEDEC trước:

```gdb
p/x application_memory_manufacturer_id
p/x application_memory_type_id
p/x application_memory_capacity_id
```

Nếu muốn trace transaction:

```gdb
break w25q64_init
break w25q64_sector_erase
break w25q64_page_program
continue
```

Không dùng breakpoint giữa erase busy polling nếu muốn đo timing thực.


## 22. Test procedure

1. Nối đúng 6 dây.
2. Flash/reset.
3. Quan sát PC13.
4. Nếu heartbeat, mở GDB và xác nhận flags.
5. Xác nhận JEDEC trước khi đánh giá erase/program.
6. Có thể reset lại, nhưng nhớ mỗi reset erase lại sector cuối.

## 23. Troubleshooting JEDEC

### `00 00 00`

Thường kiểm tra:

- MISO/D1 bị kéo LOW,
- wiring D1↔PA6,
- nguồn/common ground,
- CS waveform.

### `FF FF FF`

Thường kiểm tra:

- MISO hở,
- CS không xuống LOW,
- device không được chọn/không có nguồn,
- nhầm D0/D1.

### ID đúng nhưng program/erase fail

SPI cơ bản đã hoạt động. Tập trung:

- WEL,
- BUSY,
- CS high giữa commands,
- address,
- page boundary,
- protection/status bits nếu module đã được cấu hình trước đó.

## 24. Logic analyzer

Decode SPI mode 0, MSB-first.

JEDEC transaction expected:

```text
CS↓
MOSI: 9F FF FF FF
MISO: xx EF 40 17
CS↑
```

Write Enable:

```text
CS↓ 06 CS↑
```

Status read:

```text
CS↓ 05 FF ... CS↑
```

## 25. Wear/endurance note

Demo erase sector cuối mỗi reset. NOR flash có finite program/erase endurance. Không dùng reset loop liên tục như soak test nếu không cần; production design nên wear-level hoặc giới hạn erase.

## 26. Bài tập mở rộng

- read arbitrary region,
- multi-page program helper,
- chip erase,
- SFDP/JEDEC capability discovery,
- protection bits,
- CRC record storage,
- log-structured storage,
- wear leveling,
- SPI DMA,
- power-loss-safe metadata.

## 27. Tài liệu liên quan

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
