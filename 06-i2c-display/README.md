# 06-i2c-display — I2C1 + SSD1306 OLED 128×64

Example này điều khiển OLED **SSD1306-compatible 128×64, giao tiếp I2C bốn chân** bằng I2C1 register-level. Firmware duy trì framebuffer 1024 byte trong RAM, render text/progress bar ở thread mode và gửi toàn bộ framebuffer lên OLED bằng I2C polling.

## 1. Mục tiêu học tập

Example minh họa:

- I2C open-drain electrical model,
- PB6/PB7 alternate-function open-drain,
- cấu hình I2C1 standard/fast mode,
- tính `CR2.FREQ`, `CCR`, `TRISE`,
- START → address → ADDR clear sequence → data → BTF → STOP,
- xử lý I2C error flags và timeout polling,
- SSD1306 command/data control byte,
- ECUAL driver với transport callback,
- framebuffer page layout,
- text rendering bằng font nhỏ,
- periodic UI refresh ngoài interrupt,
- delay power-on không phụ thuộc SysTick trong giai đoạn global IRQ chưa enable.

## 2. Hardware và wiring

OLED bốn chân:

```text
Blue Pill                 OLED
--------------------------------
GND             --------  GND
3.3V            --------  VCC
PB6 / I2C1_SCL  --------  SCL
PB7 / I2C1_SDA  --------  SDA
```

Dùng 3.3 V.

Nhiều module OLED có sẵn pull-up SCL/SDA. Nếu module không có, cần pull-up ngoài phù hợp (thường vài kΩ) lên 3.3 V.

Địa chỉ mặc định:

```text
0x3C (7-bit)
```

Nếu module strap sang địa chỉ khác, project cho phép đổi config, ví dụ `0x3D`.

## 3. Expected display

Firmware render các nội dung:

```text
STM32F103
I2C SSD1306

UPTIME <seconds>
SECONDS

[ progress bar ]
```

Progress bar tăng/giảm theo thời gian.

## 4. Compile-time configuration

`config/board_config.h`:

```c
#define BOARD_HSE_FREQUENCY_HZ         (8000000UL)
#define BOARD_TARGET_CLOCK_HZ           (72000000UL)
#define BOARD_TIMEBASE_HZ               (1000UL)

#define BOARD_DISPLAY_I2C_ADDRESS_7BIT  (0x3CU)
#define BOARD_DISPLAY_I2C_CLOCK_HZ      (400000UL)
#define BOARD_DISPLAY_POWER_ON_DELAY_MS (100UL)
```

`config/application_config.h`:

```c
#define DISPLAY_DEMO_UPDATE_PERIOD_MS (100UL)
#define DISPLAY_DEMO_PROGRESS_STEP    (2U)
```

MCAL polling timeout:

```c
#define MCAL_I2C_POLL_TIMEOUT_CYCLES (500000UL)
```

## 5. Pin configuration

BSP:

```text
PB6 = I2C1_SCL = AF open-drain
PB7 = I2C1_SDA = AF open-drain
```

Open-drain là bắt buộc theo topology bus I2C: device chỉ kéo line LOW; mức HIGH do pull-up tạo.

## 6. I2C clock — normal 72 MHz system clock

RCC configuration tạo:

```text
SYSCLK = 72 MHz
PCLK1  = 36 MHz
```

Requested bus:

```text
400 kHz fast mode
duty cycle 2
```

MCAL fast-mode calculation:

```text
CCR = ceil(PCLK1 / (3 × SCL))
    = ceil(36 MHz / 1.2 MHz)
    = 30
```

`CR2.FREQ`:

```text
36 MHz → FREQ = 36
```

Fast-mode rise-time:

```text
TRISE = floor(FREQ_MHz × 300 ns / 1000 ns) + 1
      = floor(36 × 0.3) + 1
      = 11
```

## 7. HSI fallback

Khi system fallback về HSI 8 MHz:

```text
PCLK1 = 8 MHz
```

MCAL dùng rounding-up cho CCR để actual SCL không vượt requested max.

Approx:

```text
CCR = ceil(8 MHz / (3 × 400 kHz)) = 7
SCL ≈ 8 MHz / (3 × 7) ≈ 381 kHz
```

Vì BSP truyền actual PCLK1, driver không hard-code 36 MHz.

## 8. I2C initialization sequence

`mcal_i2c_init()`:

1. validate peripheral/bus clock,
2. enable I2C1 clock,
3. pulse I2C1 reset qua APB1RSTR,
4. disable/clear CR1,
5. set `CR2.FREQ`,
6. set mandatory OAR1 bit 14,
7. tính CCR/TRISE,
8. clear software-visible error bits,
9. set `CR1.PE`,
10. bảo đảm bus không BUSY.

## 9. Write transaction

OLED chỉ cần master transmitter trong example.

`mcal_i2c_master_write_prefixed()` thực hiện:

```text
wait BUSY = 0
    ↓
CR1.START = 1
    ↓
wait SR1.SB
    ↓
DR = 7-bit address << 1 (write)
    ↓
wait SR1.ADDR
    ↓
read SR1 then SR2 để clear ADDR
    ↓
wait TXE
    ↓
DR = prefix/control byte
    ↓
for each payload byte:
    wait TXE
    DR = data
    ↓
wait BTF
    ↓
CR1.STOP = 1
```

Nếu error/timeout, code phát STOP và clear error state theo implementation.

## 10. SSD1306 control bytes

BSP memory/display bus thêm prefix:

```text
0x00 → command stream
0x40 → data stream
```

ECUAL chỉ gọi transport với `data_mode`; nó không biết I2C1/PB6/PB7.

## 11. ECUAL transport design

`ssd1306_transport_t` chứa callback:

```c
write(data_mode, data, length)
delay_ms(delay)
```

`Display Service` là composition point tạo transport từ BSP functions rồi truyền vào `ssd1306_init()`.

Lợi ích:

- SSD1306 driver không include BSP header về pin/peripheral,
- có thể thay I2C transport bằng transport khác nếu contract tương thích,
- Application chỉ gọi Display Service.

## 12. Power-on delay

Bốn chân OLED không expose reset pin. Driver cần chờ module ổn định trước init commands.

Điểm quan trọng của implementation hiện tại:

```text
main() disable global IRQ
    ↓
system_init()
    ↓
display_service_init()
    ↓
ssd1306_init()
    ↓
delay 100 ms
```

Ở thời điểm đó SysTick interrupt chưa thể tăng tick vì global IRQ còn disabled. Do đó `board_display_bus_delay_ms()` dùng `mcal_delay_busy_ms()` dựa trên NOP loop, không dựa vào SysTick.

Sau khi `system_init()` hoàn tất và IRQ được enable, Application mới dùng Time Service/SysTick cho UI refresh.

## 13. SSD1306 initialization

Driver gửi command sequence gồm các cấu hình chính:

- display off,
- display clock,
- multiplex 64 rows,
- display offset/start line,
- internal charge pump,
- horizontal addressing mode,
- segment remap,
- COM scan remap,
- COM pin config,
- contrast,
- pre-charge,
- VCOMH,
- normal display,
- scrolling off,
- display on.

Sau init driver clear framebuffer và update toàn màn hình.

## 14. Framebuffer layout

Kích thước:

```text
128 × 64 / 8 = 1024 byte
```

SSD1306 dùng page theo 8 vertical pixels:

```text
8 pages × 128 columns
```

Index:

```text
index = x + (y / 8) × 128
bit   = 1 << (y % 8)
```

`ssd1306_draw_pixel()` set/clear bit tương ứng.

## 15. Text renderer

Driver chứa glyph table 5×7 cho:

- space/punctuation cần thiết,
- digits 0..9,
- uppercase A..Z.

Mỗi character advance 6 pixel: 5 cột glyph + 1 cột spacing.

Không có full Unicode/font engine.

## 16. Progress bar

Application giữ:

```c
application_display_progress_percent
```

Step:

```text
+2 hoặc -2 mỗi 100 ms
```

Từ 0 → 100 cần ~50 update ≈ 5 s; full up/down cycle ≈ 10 s.

## 17. Display update

`ssd1306_update()`:

1. gửi column address 0..127,
2. gửi page address 0..7,
3. gửi toàn bộ 1024-byte framebuffer.

Đây là full-frame update, đơn giản nhưng không tối ưu bandwidth. Với 400 kHz và update 100 ms, demo vẫn có budget hợp lý.

## 18. Error handling

Nếu display init fail:

```text
display_service_init false
 → system_init false
 → panic
```

Nếu update runtime fail:

```text
application_display_error_count++
g_display_operational = false
```

Application ngừng gửi frame tiếp theo.

Debug globals:

```gdb
p application_display_progress_percent
p application_display_update_count
p application_display_error_count
```

## 19. Interrupt policy

Strong:

```text
SysTick_Handler
```

Weak/default:

```text
I2C1_EV_IRQHandler
I2C1_ER_IRQHandler
```

I2C hoàn toàn polling.

## 20. Kiến trúc

```text
Application
   ├─> Display Service
   │      ├─> SSD1306 ECUAL
   │      └─> Board Display Bus
   │              └─> MCAL I2C + GPIO
   │
   └─> Time Service
          └─> Board Timebase
                 └─> MCAL SysTick
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


Debug I2C init/update:

```gdb
break mcal_i2c_master_write_prefixed
break ssd1306_init
continue
```

Nếu muốn xem runtime status:

```gdb
p application_display_update_count
p application_display_error_count
```


## 21. Test procedure

1. Nối đúng 4 chân.
2. Flash firmware.
3. OLED phải bật và hiển thị text.
4. Uptime phải tăng.
5. Progress bar phải animate.
6. Halt GDB và kiểm tra error count = 0.
7. Nếu không hiển thị, thử xác nhận địa chỉ module là 0x3C/0x3D.

## 22. Troubleshooting

### OLED hoàn toàn đen

Kiểm tra:

- VCC/GND,
- PB6/PB7 không đảo,
- pull-up,
- địa chỉ,
- I2C BUSY stuck,
- init return false.

### I2C BUSY luôn set

Có thể SDA/SCL bị giữ LOW. Kiểm tra wiring/module/pull-up. Implementation hiện chưa có bus-recovery bằng 9 SCL pulses.

### Text lệch/đảo

SSD1306-compatible modules có thể khác panel geometry/controller. Init sequence hiện dành cho 128×64 common configuration.

### Update error sau khi chạy

Inspect `application_display_error_count`, I2C SR1 error bits bằng debugger/logic analyzer. Dây dài và pull-up không phù hợp dễ gây lỗi ở 400 kHz.

## 23. Bài tập mở rộng

- dirty rectangles/partial update,
- font lowercase,
- bitmap/icon,
- I2C interrupt/DMA,
- bus recovery,
- brightness API,
- multiple pages/screens,
- SH1106 variant support.

## 24. Tài liệu liên quan

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
