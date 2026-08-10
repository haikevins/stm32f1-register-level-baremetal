# Porting Guide — 04-uart-interrupt-ring-buffer

## 1. Những phần có thể giữ nguyên

Khi đổi board/USART nhưng vẫn muốn byte-stream API:

```text
app/
services/serial_service.*
```

nên giữ nguyên nếu contract không đổi.

## 2. Đổi USART instance

Cần cập nhật:

- enum instance MCAL,
- register pointer,
- RCC clock-enable mask,
- IRQ number,
- IRQ priority mapping,
- strong ISR đúng tên vector,
- BSP pin mapping,
- bus clock source.

USART2/3 nằm APB1 trên STM32F103.

## 3. Đổi buffer size

Sửa:

```c
MCAL_USART_RX_BUFFER_SIZE
MCAL_USART_TX_BUFFER_SIZE
```

Ràng buộc:

- >= 2,
- power of two,
- <= 32768,
- usable = size - 1.

Tăng buffer tăng RAM consumption nhưng không thay thế việc loại blocking task.

## 4. Đổi IRQ priority

Xem toàn bộ hệ thống, không chọn priority cô lập. Nếu ghép với ADC DMA/EXTI, xác định peripheral nào có latency budget chặt hơn.

## 5. Đổi baud/data format

Baud có thể đổi config trực tiếp. 9-bit/parity/2-stop cần mở rộng register config.

## 6. Đổi TX/RX pins

Nếu dùng remap:

- AFIO enable/map,
- đúng GPIO mode,
- không xung đột SWD/JTAG.

## 7. Port sang DMA UART

Có thể giữ Serial Service API nhưng lower-layer semantics thay đổi. Cần xác định:

- ring ownership,
- DMA half/full event,
- TX completion,
- abort/error handling.

Không để DMA ISR gọi Application.

## 8. Validation concurrency

Stress test bắt buộc:

- continuous RX,
- simultaneous TX/RX,
- TX ring full,
- RX ring full,
- injected framing/noise nếu có thiết bị,
- critical-section race bằng load cao,
- wrap index nhiều vòng.

## 9. Symbol validation

Sau build:

```bash
arm-none-eabi-nm build/firmware.elf | grep USART1_IRQHandler
```

Handler interrupt-driven phải là strong text symbol, không weak default.

## 10. Common pitfalls

- quên tắt TXEIE khi ring empty → interrupt storm,
- enqueue byte nhưng quên kick TXEIE,
- shared index bị nhiều writer,
- size không power-of-two nhưng vẫn dùng mask,
- xử lý echo trong ISR,
- clear error flags sai SR→DR sequence,
- nhầm APB clock.
