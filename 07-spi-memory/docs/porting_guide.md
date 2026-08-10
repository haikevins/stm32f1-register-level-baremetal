# Porting Guide — 07-spi-memory

## 1. Đổi pin SPI/CS trên STM32F103

Cập nhật `board_pins.h` và bảo đảm alternate-function mapping hợp lệ.

Default:

```text
PA4 CS GPIO
PA5 SPI1_SCK
PA6 SPI1_MISO
PA7 SPI1_MOSI
```

Nếu dùng remap hoặc SPI2, cần thay nhiều hơn pin macro.

## 2. Đổi sang SPI2

SPI2 nằm APB1. Cần:

- SPI2 base address,
- RCC APB1 enable bit,
- MCAL instance mapping,
- actual APB1 clock,
- pin mapping PB13/PB14/PB15 theo board,
- CS GPIO.

Memory Service/W25Q64 ECUAL có thể giữ nguyên.

## 3. Đổi SPI frequency

Sửa:

```c
BOARD_MEMORY_SPI_MAX_HZ
```

MCAL chọn prescaler không vượt max.

Khi tăng tốc:

- dây ngắn,
- common ground tốt,
- signal integrity,
- module level shifting nếu có,
- datasheet timing.

Không giả định module breakout chịu cùng max clock như bare chip.

## 4. Dùng flash dung lượng khác

Nếu vẫn command-compatible W25Q series:

- update geometry,
- capacity ID expectation,
- address width nếu >16 MiB cần 4-byte addressing,
- sector/page sizes nếu khác,
- test sector address.

Không để Application test address vượt memory geometry.

## 5. Đổi test sector

Sửa:

```c
MEMORY_DEMO_TEST_SECTOR_ADDRESS
```

Chọn sector không chứa dữ liệu cần giữ.

Address được sector erase align xuống 4 KiB, nên dùng address sector-aligned để tránh nhầm.

## 6. Multi-page programming

Public `memory_service_program_page()` giữ restriction không vượt một 256-byte page.

Nếu cần write buffer dài:

1. split theo page boundary ở Service/ECUAL helper,
2. WREN cho mỗi page program,
3. poll BUSY mỗi page,
4. xử lý partial first/last page.

## 7. Runtime asynchronous operation

Hiện erase có thể block nhiều poll iterations. Để dùng trong responsive system:

- chuyển W25Q64 operation thành state machine,
- issue command một lần,
- poll status từ `application_process`/service process,
- dùng deadline ms,
- không busy-loop dài.

## 8. Shared SPI bus

Nếu có nhiều device:

- mỗi device CS riêng,
- bus config compatible hoặc reconfigure per transaction,
- serialize transfers,
- không để hai CS low cùng lúc,
- thiết kế bus arbitration ở BSP/Service, không trong Application raw GPIO.

## 9. Port sang MCU khác

Giữ W25Q64 ECUAL và Memory Service; thay:

```text
MCAL SPI
MCAL GPIO
RCC
BSP mapping
Platform registers
```

## 10. Validation checklist

- CS idle HIGH,
- SPI mode 0 waveform,
- JEDEC ID đúng,
- WEL set sau WREN,
- BUSY set/clear hợp lý sau erase/program,
- read-back exact,
- SPI IRQ weak nếu polling,
- layer checker pass.

## 11. Common pitfalls

- D0/D1 đảo,
- CS floating/LOW trong boot,
- page write vượt boundary,
- không WREN trước mỗi program/erase,
- không poll BUSY,
- erase nhầm sector,
- dùng 5 V logic,
- timeout theo SysTick khi IRQ chưa enable.
