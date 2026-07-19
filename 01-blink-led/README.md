# STM32F103 Bare-Metal Layered Template

Template cho **STM32F103C8T6 / Blue Pill**, viết bằng C11 và assembly, không dùng HAL,
LL, SPL, libopencm3, Arduino Core hoặc RTOS.

Template minh họa chuỗi phụ thuộc kín:

```text
Application -> Services -> BSP/ECUAL -> MCAL -> Device/Architecture -> Hardware
```

`system/` là **composition root** duy nhất được phép khởi tạo và kết nối nhiều tầng.
Startup/linker/build scripts là infrastructure, không phải application layer.

## Mẫu chạy sẵn

Sau khi flash, LED PC13 của Blue Pill nhấp nháy non-blocking mỗi 500 ms:

```text
Application
  -> Indication Service
    -> Board LED
      -> MCAL GPIO
        -> GPIOC registers
```

Time path:

```text
Application
  -> Time Service
    -> Board Timebase
      -> MCAL SysTick
        -> Cortex-M3 SysTick registers
```

## Yêu cầu

- GNU Arm Embedded Toolchain: `arm-none-eabi-gcc`
- GNU Make
- OpenOCD
- ST-Link hoặc probe tương thích

## Build

```bash
make
```

Output:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

## Flash

```bash
make flash
```

## Debug

Terminal 1:

```bash
make debug-server
```

Terminal 2:

```bash
make debug
```

## Dùng repository làm template mới

```bash
git clone <template-url> my-project
cd my-project
rm -rf .git
git init
git add .
git commit -m "Initial project from STM32F103 layered template"
```

Sau đó:

1. Đổi `PROJECT` trong `Makefile`.
2. Sửa `config/application_config.h`.
3. Thay logic trong `app/src/application.c`.
4. Thêm service ở `services/`.
5. Thêm thiết bị ngoài ở `ecual/`.
6. Thêm peripheral driver register-level ở `mcal/`.
7. Khi đổi board, tạo BSP mới thay vì sửa application.

## Quy tắc include bắt buộc

- `app/` chỉ include API của `services/` và kiểu thuần từ `common/`.
- `services/` chỉ include `bsp/`, `ecual/` và `common/`.
- `bsp/` và `ecual/` chỉ include `mcal/` và `common/`.
- `mcal/` chỉ include `platform/device`, `platform/arch` và `common/`.
- `platform/` không phụ thuộc tầng phía trên.
- ISR trong MCAL không gọi callback application.

Chi tiết xem `docs/architecture.md` và `docs/porting_guide.md`.

## Ghi chú phần cứng

Linker script mặc định khai báo đúng cấu hình STM32F103C8T6:

- Flash: 64 KiB tại `0x08000000`
- SRAM: 20 KiB tại `0x20000000`
- Cortex-M3, tối đa 72 MHz

Clock mặc định thử HSE 8 MHz x9 để đạt 72 MHz. Nếu HSE không sẵn sàng, MCAL tự quay về HSI 8 MHz để firmware vẫn chạy.
