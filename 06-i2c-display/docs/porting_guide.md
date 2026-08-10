# Porting Guide — 06-i2c-display

## 1. Đổi địa chỉ OLED

Sửa:

```c
BOARD_DISPLAY_I2C_ADDRESS_7BIT
```

Dùng 7-bit address, không truyền giá trị đã shift trái.

## 2. Đổi bus speed

Sửa:

```c
BOARD_DISPLAY_I2C_CLOCK_HZ
```

Driver hỗ trợ tối đa 400 kHz theo guard hiện tại.

Sau đổi kiểm tra:

- PCLK1 1..36 MHz,
- `CR2.FREQ`,
- CCR không vượt 12-bit,
- TRISE hợp lệ,
- signal integrity/pull-up.

## 3. Đổi I2C pins

PB6/PB7 là default I2C1 mapping. Nếu remap hoặc I2C2:

- update BSP pins,
- update peripheral instance MCAL,
- RCC clock/reset bits,
- base address,
- AFIO remap nếu cần,
- bus clock source.

## 4. Đổi controller OLED

Nếu là SH1106 hoặc panel geometry khác, không chỉ đổi address. Tạo ECUAL driver mới hoặc parameterize:

- init commands,
- page/column addressing,
- resolution,
- framebuffer width/height.

## 5. Đổi resolution

Hiện macros:

```c
SSD1306_WIDTH  = 128
SSD1306_HEIGHT = 64
```

Frame buffer size và addressing commands phụ thuộc các giá trị này, nhưng init multiplex/COM config hiện hard-code 64-row profile. Cần cập nhật đồng bộ.

## 6. Thêm reset pin

Module 4-pin không có reset. Nếu board khác expose RESET:

- map pin ở BSP,
- tạo reset sequence BSP,
- giữ SSD1306 device protocol độc lập nếu có thể.

## 7. Thay power-on delay

Hiện init chạy khi IRQ disabled nên delay callback phải hoạt động không cần interrupt.

Nếu chuyển init sau global IRQ enable, có thể dùng timebase, nhưng phải document lifecycle rõ.

## 8. Port sang MCU khác

Giữ SSD1306 ECUAL và Display Service; thay:

- MCAL I2C,
- GPIO AF mapping,
- RCC,
- BSP.

## 9. Verification checklist

- bus idle high,
- START/address ACK,
- control byte 0x00 command ACK,
- control byte 0x40 data ACK,
- screen init,
- full-frame update,
- runtime error count = 0,
- I2C IRQ handlers vẫn weak nếu polling.

## 10. Logic analyzer checklist

Decode I2C:

```text
address 0x3C write
0x00 + commands
...
address 0x3C write
0x40 + framebuffer
```

Nếu analyzer hiển thị 0x78, kiểm tra tool đang hiển thị 8-bit address byte hay 7-bit address.

## 11. Common pitfalls

- nhầm 0x3C với 0x78,
- thiếu pull-up,
- dùng push-pull cho SCL/SDA,
- clear ADDR sai sequence,
- STOP trước BTF cuối,
- dùng SysTick delay khi IRQ còn disabled,
- full-frame update quá nhanh làm chiếm bus.
