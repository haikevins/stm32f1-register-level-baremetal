# STM32F1 Register-Level Bare-Metal Examples

Thư mục này chứa chuỗi example độc lập cho **STM32F103C8T6 Blue Pill**, được sắp xếp từ nền tảng cơ bản đến pipeline peripheral phức tạp hơn. Mỗi example có Makefile, startup, linker script, platform register map và tài liệu riêng để có thể build/flash/debug mà không cần phụ thuộc project khác.

## 1. Triết lý của chuỗi example

Các example được thiết kế để luyện ba kỹ năng song song:

- **Register-level programming**: hiểu chính xác peripheral register, flag, clock enable, IRQ và data path.
- **Firmware architecture**: Application không biết pin/register; BSP/ECUAL/MCAL có trách nhiệm rõ ràng.
- **Debugability**: có symbol debug, error counter, layer checker và luồng xử lý dễ trace.

Không dùng:

- STM32 HAL
- STM32 LL
- Standard Peripheral Library
- libopencm3
- Arduino Core
- RTOS
- dynamic allocation

## 2. Lộ trình học đề xuất

### Bước 1 — GPIO và timebase

[`01-blink-led`](01-blink-led/) tập trung vào:

- HSE/PLL + HSI fallback,
- GPIO output,
- SysTick 1 ms,
- periodic scheduling không blocking.

Đây là example nên đọc đầu tiên để hiểu startup → board → service → application.

### Bước 2 — Interrupt input

[`02-gpio-input-interrupt`](02-gpio-input-interrupt/) thêm:

- GPIO pull-up,
- AFIO EXTI mapping,
- EXTI falling edge,
- NVIC priority/enable,
- ISR event capture,
- debounce ở thread mode.

### Bước 3 — UART polling

[`03-uart-polling`](03-uart-polling/) giới thiệu:

- USART1,
- baud-rate divider,
- RXNE/TXE polling,
- error flag,
- API `try_read`/`try_write`.

### Bước 4 — UART interrupt + ring buffer

[`04-uart-interrupt-ring-buffer`](04-uart-interrupt-ring-buffer/) chuyển data path sang interrupt:

- RX ring,
- TX ring,
- RXNE/TXE interrupt,
- overflow/error counters,
- critical section ngắn để khởi động TX an toàn.

### Bước 5 — Timer PWM

[`05-timer-pwm`](05-timer-pwm/) giới thiệu:

- TIM2_CH1,
- PWM mode 1,
- PSC/ARR/CCR,
- preload,
- quy tắc x2 timer clock khi APB prescaler khác 1.

### Bước 6 — I2C + external device

[`06-i2c-display`](06-i2c-display/) thêm:

- I2C1 fast mode,
- open-drain pins,
- START/ADDR/TXE/BTF/STOP sequence,
- ECUAL SSD1306,
- framebuffer 1024 byte,
- transport callback giữa Service và ECUAL/BSP.

### Bước 7 — SPI flash

[`07-spi-memory`](07-spi-memory/) thêm:

- SPI1 full-duplex polling,
- software CS,
- JEDEC ID,
- W25Q64 status polling,
- sector erase,
- page program,
- read-back verify.

### Bước 8 — ADC + DMA pipeline

[`08-adc-dma`](08-adc-dma/) kết hợp:

- TIM3 TRGO,
- ADC1 regular conversion,
- ADC calibration,
- DMA1 Channel 1 circular,
- half/full-transfer interrupt,
- block processing ngoài ISR.

## 3. Bảng tổng hợp

| Example | Peripheral | Pin chính | IRQ mạnh | Output/kiểm tra |
|---|---|---|---|---|
| 01 | GPIOC, SysTick | PC13 | `SysTick_Handler` | LED blink 500 ms |
| 02 | GPIOA/C, AFIO, EXTI, SysTick | PA0, PC13 | `EXTI0_IRQHandler`, `SysTick_Handler` | Nhấn nút toggle LED |
| 03 | USART1 | PA9/PA10 | Không dùng USART IRQ | Greeting + echo |
| 04 | USART1, NVIC | PA9/PA10 | `USART1_IRQHandler` | Greeting + buffered echo |
| 05 | TIM2_CH1, SysTick | PA0 | `SysTick_Handler` | PWM fade |
| 06 | I2C1, SSD1306, SysTick | PB6/PB7 | `SysTick_Handler` | OLED text/progress |
| 07 | SPI1, W25Q64, SysTick | PA4..PA7 | `SysTick_Handler` | JEDEC + erase/program/verify |
| 08 | TIM3, ADC1, DMA1 | PA0 | `DMA1_Channel1_IRQHandler` | ADC statistics + PC13 threshold |

## 4. Wiring tổng hợp

### SWD — dùng cho tất cả example

```text
ST-Link             Blue Pill
-----------------------------
SWDIO      -------  PA13
SWCLK      -------  PA14
GND        -------  GND
3.3V REF   -------  3.3V
```

### Example 02 — button

```text
PA0 ---- push button ---- GND
```

PA0 dùng pull-up nội.

### Example 03/04 — USB-UART

```text
PA9  USART1_TX  ---> USB-UART RX
PA10 USART1_RX  <--- USB-UART TX
GND             ---- USB-UART GND
```

Adapter phải dùng logic 3.3 V.

### Example 05 — PWM LED

```text
PA0 / TIM2_CH1 ---- 330 Ω ---- LED ---- GND
```

### Example 06 — OLED I2C

```text
Blue Pill              OLED
---------------------------
GND        ----------  GND
3.3V       ----------  VCC
PB6        ----------  SCL
PB7        ----------  SDA
```

### Example 07 — W25Q64

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

### Example 08 — analog input

```text
3.3V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

## 5. Build/flash chung

Vào thư mục example:

```bash
cd 04-uart-interrupt-ring-buffer
```

Kiểm tra dependency:

```bash
make check-layers
```

Build sạch:

```bash
make clean
make
```

Flash:

```bash
make flash
```

Xem size:

```bash
make size
```

## 6. Debug chung

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

OpenOCD dùng SWD và:

```tcl
reset_config none
adapter speed 1000
```

Điều này phù hợp với ST-Link clone không nối NRST.

## 7. Clock behavior chung

Phần lớn example dùng:

```text
HSE 8 MHz → PLL x9 → SYSCLK 72 MHz
```

Nếu HSE/PLL không thành công:

```text
HSI 8 MHz fallback
```

Các module phụ thuộc clock được truyền tần số thực tế từ RCC layer:

- USART1 dùng APB2 clock,
- I2C1 dùng APB1 clock,
- SPI1 dùng APB2 clock,
- TIM2/TIM3 dùng timer clock,
- ADC chọn prescaler để giữ ADC clock trong giới hạn cấu hình.

## 8. Kiến trúc và layer checker

Dependency chuẩn:

```text
app
 ↓
services
 ↓
bsp / ecual
 ↓
mcal
 ↓
platform
```

`tools/scripts/check_layers.py` quét include project-local và fail build nếu có dependency đi ngược.

Ví dụ Application hợp lệ:

```c
#include "serial_service.h"
```

Ví dụ Application không hợp lệ:

```c
#include "mcal_usart.h"
#include "stm32f103xb.h"
```

## 9. Interrupt ownership và thread mode

Khi đọc source, phân biệt:

- **interrupt context**: acknowledge hardware, capture dữ liệu/event,
- **thread mode**: debounce, echo policy, statistics, display render, LED state machine.

Không đánh giá chất lượng ISR chỉ bằng số dòng; điều quan trọng là ISR không kéo dependency tầng trên và không thực hiện công việc có latency khó kiểm soát.

## 10. `system_idle()` trong chuỗi example

Tất cả numbered examples trong archive hiện tại dùng:

```c
cortex_m3_nop();
```

thay vì `WFI`. Lý do thực tế là debug với ST-Link không có NRST dễ dự đoán hơn. Nếu tối ưu power, cần đánh giá riêng sleep/wakeup và debug workflow.

## 11. Cách chọn example để mở rộng

Nếu cần xây một project mới:

- UART command shell → bắt đầu từ 04.
- PWM actuator → bắt đầu từ 05.
- OLED UI → bắt đầu từ 06.
- External SPI NOR → bắt đầu từ 07.
- Sensor analog streaming → bắt đầu từ 08.
- Một peripheral chưa có → dùng `template/` và tham khảo MCAL gần nhất.

## 12. Quy tắc khi copy code giữa example

Không copy cả module nếu không cần. Thay vào đó:

1. xác định API public,
2. copy MCAL/platform dependency tối thiểu,
3. update `board_init()`,
4. chạy layer checker,
5. kiểm tra IRQ symbol mạnh/yếu,
6. kiểm tra clock input,
7. kiểm tra linker size,
8. cập nhật README/architecture/porting guide.

## 13. Lưu ý an toàn phần cứng

- GPIO/USART/I2C/SPI trong các wiring này là logic 3.3 V.
- Không đưa tín hiệu analog PA0 vượt rail nguồn.
- Không short GPIO output trực tiếp vào GND/3.3 V.
- Example W25Q64 xóa sector cuối mỗi reset; không lưu dữ liệu quan trọng tại sector đó khi chạy demo.
- Với OLED I2C, nếu module không có pull-up thì cần pull-up ngoài lên 3.3 V.

## 14. Tài liệu chi tiết

Trong mỗi example:

- `README.md`: hướng dẫn build/run/test.
- `docs/architecture.md`: trách nhiệm module, data flow, interrupt/concurrency.
- `docs/porting_guide.md`: checklist đổi board/pin/peripheral/clock.

Đọc cả ba file trước khi sửa kiến trúc của example.
