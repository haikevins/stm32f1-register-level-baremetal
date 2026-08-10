# Porting Guide — 03-uart-polling

## 1. Đổi chân nhưng vẫn USART1

STM32F1 pin remap không phải mọi pin đều tự do. Nếu dùng mapping remap của USART1, cần:

- enable AFIO,
- cấu hình `AFIO_MAPR`,
- update BSP pins,
- giữ TX AF push-pull và RX input phù hợp.

Không chỉ đổi macro pin nếu hardware mapping không hỗ trợ.

## 2. Đổi sang USART2/USART3

Cần cập nhật:

1. `mcal_usart_instance_t`,
2. base address trong `stm32f103xb_memory.h`,
3. pointer/register mapping,
4. RCC enable bit,
5. board pin mapping,
6. peripheral clock source.

USART1 nằm APB2; USART2/3 nằm APB1, nên clock argument khác.

## 3. Đổi baud rate

Sửa:

```c
#define BOARD_UART_BAUD_RATE ...
```

MCAL tính divider runtime từ peripheral clock.

Validation:

- divider >= 16,
- divider <= 0xFFFF,
- actual baud error chấp nhận được.

## 4. Đổi clock tree

Không hard-code 72 MHz trong UART driver. Bảo đảm RCC layer trả đúng bus clock thực tế.

Nếu APB prescaler thay đổi, USART clock **không** dùng quy tắc timer x2.

## 5. Đổi data format

Hiện code dựa reset defaults cho 8N1/no parity. Muốn 9-bit/parity/stop khác cần mở rộng MCAL config/API và thêm CR1/CR2 bit definitions.

Không đưa format-specific register bits vào Service.

## 6. Thêm timeout blocking API

Không nên thay `try_*` bằng hidden busy-wait nếu muốn giữ architecture non-blocking. Nếu cần blocking wrapper, đặt ở tầng có policy timeout rõ ràng và document latency.

## 7. Port sang MCU family khác

Giữ Service API, thay:

- device register map,
- USART MCAL,
- RCC/clock query,
- BSP pin AF setup.

## 8. Test sau port

- greeting đúng ký tự,
- echo hai chiều,
- baud đo bằng logic analyzer nếu cần,
- RX/TX counters tăng,
- error flags bằng 0 trong điều kiện sạch,
- layer checker pass,
- USART IRQ vẫn weak nếu vẫn polling.

## 9. Common pitfalls

- nối TX-TX thay vì TX-RX,
- dùng UART adapter 5 V,
- nhầm PCLK1/PCLK2,
- quên AF output mode,
- không đọc DR sau error,
- dùng blocking loop khiến super-loop treo.
