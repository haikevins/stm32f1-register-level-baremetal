# STM32F103 Register Level Bare-Metal Template

Khung project tối thiểu cho **STM32F103C8T6 / Blue Pill**, sử dụng C11 và assembly,
không dùng HAL, LL, SPL, libopencm3, Arduino Core hoặc RTOS.

Repository này **không chứa example chức năng**. Firmware mặc định chỉ:

```text
Reset
  -> khởi tạo .data/.bss
  -> board_init() rỗng
  -> application_init() rỗng
  -> super-loop gọi application_process() rỗng
  -> WFI
```

Không có sẵn blink LED, GPIO, SysTick, UART, event service hoặc driver thiết bị ngoài.
Người dùng tự thêm các module cần thiết theo kiến trúc:

```text
Application -> Services -> BSP/ECUAL -> MCAL -> Device/Architecture -> Hardware
```

`system/` là composition root duy nhất được phép khởi tạo nhiều tầng.

## Build

Yêu cầu:

- `arm-none-eabi-gcc`
- GNU Make
- OpenOCD khi flash/debug
- `arm-none-eabi-gdb` hoặc `gdb-multiarch` khi debug

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

## Flash và debug

```bash
make flash
```

Debug bằng hai terminal:

```bash
# Terminal 1
make debug-server
```

```bash
# Terminal 2
make debug
```

## Tạo project mới từ template

```bash
git clone <template-url> my-project
cd my-project
rm -rf .git
git init
git add .
git commit -m "Initial project from STM32F103 layered skeleton"
```

Sau đó:

1. Đổi `PROJECT` trong `Makefile` hoặc chạy `make PROJECT=my_firmware`.
2. Viết product logic trong `app/`.
3. Tạo service trong `services/` khi application cần một API cấp cao.
4. Tạo board abstraction trong `bsp/bluepill/`.
5. Tạo external-device driver trong `ecual/`.
6. Tạo peripheral driver register-level trong `mcal/`.
7. Mở rộng register map trong `platform/device/stm32f103xb/`.
8. Cập nhật `system/system_init.c` để nối các module tại composition root.

Các thư mục chưa có module được giữ trên Git bằng `.gitkeep`.

Xem thêm:

- `docs/architecture.md`
- `docs/adding_a_module.md`
- `docs/porting_guide.md`
