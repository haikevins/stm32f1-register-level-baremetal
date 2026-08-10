# Porting Guide — 02-gpio-input-interrupt

## 1. Đổi button sang pin khác cùng STM32F103

Cập nhật `board_pins.h`:

```c
BOARD_USER_BUTTON_PORT
BOARD_USER_BUTTON_PIN
BOARD_USER_BUTTON_ACTIVE_LEVEL
BOARD_USER_BUTTON_EXTI_LINE
BOARD_USER_BUTTON_IRQ_PRIORITY
```

Pin number và EXTI line phải tương ứng nếu dùng mapping trực tiếp.

Ví dụ PB8:

```text
GPIO port = B
pin = 8
EXTI line = 8
IRQ = EXTI9_5
```

MCAL generic sẽ tự map grouped IRQ dựa trên line.

## 2. Đổi polarity

Nếu button nối 3.3 V khi nhấn:

- đổi pull xuống,
- active level thành HIGH,
- trigger thành rising.

Hiện `board_button_init()` hard-code `MCAL_EXTI_TRIGGER_FALLING`, nên port active-high phải sửa BSP, không chỉ macro.

## 3. Dùng pull-up ngoài

Có thể cấu hình input floating thay vì input pull nếu board có điện trở ngoài. Quyết định này thuộc BSP.

## 4. Thay debounce

`BUTTON_SERVICE_DEBOUNCE_TIME_MS` thuộc Service vì đây là policy xử lý tín hiệu chứ không phải register setting.

Khi đổi giá trị, test:

- click nhanh,
- giữ nút,
- bounce mạnh,
- EMI/noise nếu dây dài.

## 5. Đổi IRQ priority

Priority hợp lệ trong implementation: 0..15, được shift vào NVIC priority byte. Xem xét quan hệ với UART/DMA IRQ nếu ghép nhiều module vào một firmware.

## 6. Port sang STM32F1 khác

Rà:

- AFIO/EXTI register layout,
- IRQ number,
- NVIC implemented priority bits,
- GPIO input pull semantics,
- linker memory.

## 7. Port sang family khác

Một số family mới dùng SYSCFG thay AFIO cho EXTI mapping. Khi đó:

- giữ Board Button API,
- thay MCAL EXTI,
- thay Platform device mapping,
- Application/Button Service giữ nguyên.

## 8. Verification checklist

- pin released đọc inactive level,
- pin pressed đọc active level,
- EXTI pending clear đúng,
- handler đúng vector,
- press event chỉ xuất hiện một lần,
- debounce chạy ngoài ISR,
- layer checker pass.

## 9. GDB checklist

```gdb
break EXTI0_IRQHandler
break button_service_take_press
continue
```

Nếu đổi sang line 8, breakpoint phải là:

```gdb
break EXTI9_5_IRQHandler
```

## 10. Logic analyzer/oscilloscope

Nếu cần xác minh bounce:

- probe PA0,
- quan sát nhiều edge trong vài ms,
- so sánh với chỉ một semantic press ở Application.

## 11. Sai lầm thường gặp

- đổi GPIO nhưng quên AFIO EXTI mapping,
- clear pending sai semantics,
- debounce bằng delay trong ISR,
- callback từ ISR lên Application,
- quên common ground,
- dùng pin đã bị debug/JTAG/peripheral khác chiếm.
