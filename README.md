# STM32F103 Register-Level Bare-Metal Template

Template này là bộ khung tối thiểu để tạo một firmware **register-level, bare-metal** cho **STM32F103C8T6 Blue Pill**. Nó đã có startup, vector table, linker script, runtime init, build system, fault handling, layer checker và skeleton module; phần peripheral/product behavior được để trống để project mới chỉ thêm đúng những module cần thiết.

## 1. Khi nào nên dùng template này

Dùng `template/` khi:

- bắt đầu một example/project register-level mới,
- muốn giữ cùng convention với chuỗi `examples/`,
- muốn tự định nghĩa register thay vì dùng HAL/LL/SPL,
- cần startup/linker/build dễ kiểm tra,
- cần architecture boundary ngay từ đầu,
- muốn tránh việc prototype nhanh rồi phải refactor toàn bộ dependency về sau.

Không nên coi template là một framework hoàn chỉnh. Nó cố tình nhỏ và explicit.

## 2. Target hiện tại

| Hạng mục | Giá trị |
|---|---|
| MCU | STM32F103C8T6 |
| Board | Blue Pill |
| CPU | Arm Cortex-M3 |
| Flash trong linker | 64 KiB |
| SRAM trong linker | 20 KiB |
| Language | C11 + GNU assembler |
| Compiler | `arm-none-eabi-gcc` |
| Build | GNU Make |
| Debug server | OpenOCD |
| Debugger | `arm-none-eabi-gdb` hoặc `gdb-multiarch` |
| Interface | SWD |
| RTOS | Không |
| Heap | Không được template sử dụng |
| HAL/LL/SPL | Không |

## 3. Cấu trúc thư mục

```text
template/
├── app/
│   ├── include/application.h
│   └── src/application.c
├── bsp/
│   └── bluepill/
│       ├── include/board.h
│       ├── include/board_pins.h
│       └── src/board.c
├── common/
│   ├── include/compiler.h
│   ├── include/project_types.h
│   └── src/
├── config/
│   └── project_config.h
├── docs/
│   ├── architecture.md
│   ├── adding_a_module.md
│   └── porting_guide.md
├── ecual/
│   ├── include/
│   └── src/
├── linker/
│   └── stm32f103c8t6.ld
├── mcal/
│   ├── include/
│   └── src/
├── platform/
│   ├── arch/cortex-m3/
│   └── device/stm32f103xb/
├── services/
│   ├── include/
│   └── src/
├── startup/
│   ├── startup_stm32f103xb.S
│   └── runtime_init.c
├── system/
│   ├── main.c
│   ├── system.h
│   ├── system_control.c
│   ├── system_fault.c
│   └── system_init.c
├── tests/host/
├── tools/
│   ├── gdb/
│   ├── openocd/
│   └── scripts/
└── Makefile
```

Các `.gitkeep` chỉ tồn tại để giữ thư mục trống trong Git.

## 4. Architecture mục tiêu

```text
Application
    ↓
Services
    ↓
BSP / ECUAL
    ↓
MCAL
    ↓
Device / Architecture
    ↓
Hardware
```

Infrastructure đứng ngoài chuỗi runtime:

```text
system/
startup/
linker/
config/
tools/
```

`system/` được phép biết nhiều tầng vì nó là **composition root**, nhưng không nên chứa product behavior.

## 5. Dependency rule

### Application

Được phụ thuộc:

```text
app
services
common
config
```

Không được include:

```text
board_*.h
mcal_*.h
stm32f103xb*.h
cortex_m3_registers.h
```

### Services

Được dùng:

```text
bsp
ecual
common
config
```

Service nên expose semantic API thay vì raw register/peripheral concept.

### BSP

Biết:

- board pin,
- active level,
- peripheral instance,
- board-specific wiring,
- actual bus resource.

BSP dùng MCAL, không gọi Service/Application.

### ECUAL

Dành cho external component như:

- SSD1306,
- W25Q64,
- sensor,
- EEPROM,
- radio.

ECUAL nên giữ device protocol, không chứa product policy.

### MCAL

Là nơi truy cập peripheral STM32 register:

- RCC,
- GPIO,
- UART,
- SPI,
- I2C,
- TIM,
- ADC,
- DMA,
- EXTI...

### Platform

Chỉ mô tả:

- core primitives,
- memory map,
- register layouts,
- IRQ numbers,
- bit masks.

## 6. Layer checker

Makefile chạy:

```bash
make check-layers
```

Script:

```text
tools/scripts/check_layers.py
```

quét `#include "..."` và xác định owner layer của header.

Mục tiêu của checker là bắt lỗi architecture sớm.

Ví dụ sai:

```c
/* app/src/application.c */
#include "mcal_gpio.h"
```

Ví dụ đúng:

```c
/* app/src/application.c */
#include "indication_service.h"
```

Checker không thay thế code review; nó chỉ kiểm tra dependency qua include mà nó nhận diện được.

## 7. Startup sequence

`startup/startup_stm32f103xb.S` tạo vector table tại section `.isr_vector`.

Hai entry đầu:

```text
initial MSP = _estack
Reset vector = Reset_Handler
```

`Reset_Handler`:

```asm
bl runtime_init
```

Nếu `runtime_init()` bất ngờ return, handler rơi vào infinite loop.

Unused IRQ handler là weak alias về `Default_Handler`.

## 8. Runtime initialization

`runtime_init.c` không dùng libc startup.

Nó:

```text
_sidata → copy → _sdata.._edata
zero _sbss.._ebss
call main()
infinite loop nếu main return
```

Điều này bảo đảm:

- initialized global/static `.data` có giá trị đúng,
- zero-initialized `.bss` đúng,
- firmware không phụ thuộc CRT của host/vendor.

## 9. Linker script

Memory:

```ld
FLASH : ORIGIN = 0x08000000, LENGTH = 64K
RAM   : ORIGIN = 0x20000000, LENGTH = 20K
```

Các section chính:

```text
.isr_vector → FLASH
.text/.rodata → FLASH
.data → RAM, load image ở FLASH
.bss → RAM NOLOAD
.noinit → RAM NOLOAD
```

Stack top:

```text
_estack = end of RAM
```

Template reserve tối thiểu:

```text
_Min_Stack_Size = 0x400
```

và linker assert static data không lấn phần stack reserve.

## 10. Vector table và interrupt extension

Startup đã khai báo vector cho STM32F103 medium-density IRQ list.

Peripheral chưa implement sẽ trỏ weak `Default_Handler`.

Khi tạo ISR mạnh với đúng tên:

```c
void USART1_IRQHandler(void)
{
    ...
}
```

linker chọn strong symbol thay weak alias.

Điều này cho phép thêm IRQ module không cần sửa vector table mỗi lần.

## 11. Fault handling

Template override mạnh:

```text
NMI_Handler
HardFault_Handler
MemManage_Handler
BusFault_Handler
UsageFault_Handler
```

và gọi:

```c
system_panic();
```

Mục tiêu là không rơi vào vendor/default loop không rõ ownership.

## 12. `main()` và composition

Flow hiện tại:

```text
disable global IRQ
    ↓
system_init()
    ↓
nếu fail → system_panic()
    ↓
enable global IRQ
    ↓
for (;;)
    application_process()
    system_idle()
```

Việc disable IRQ trong init giúp module:

- configure peripheral,
- clear state,
- configure NVIC,
- hoàn tất Application init,

trước khi interrupt bắt đầu chạy.

Nếu một module init cần delay dựa trên interrupt timebase, phải xem lại lifecycle; numbered examples 06/07 dùng busy delay trong init vì lý do này.

## 13. Idle behavior của template

Template hiện dùng:

```c
void system_idle(void)
{
    cortex_m3_wait_for_interrupt();
}
```

tức `WFI`.

Numbered examples trong repository hiện đã đổi sang `NOP` để debug dễ hơn với ST-Link không có NRST. Khi clone template, chọn policy phù hợp:

### Dùng `WFI`

Ưu:

- giảm active CPU/power,
- wake theo interrupt.

Nhược:

- peripheral polling không chạy nếu không có wake source phù hợp,
- debug workflow có thể khác.

### Dùng `NOP`

Ưu:

- super-loop luôn chạy,
- polling peripheral hoạt động,
- SWD attach dễ dự đoán.

Nhược:

- CPU luôn active.

Đây là system policy, không nên để từng Service tự gọi `WFI`.

## 14. Cortex-M3 abstraction

`cortex_m3.h` cung cấp primitive:

```text
enable IRQ
disable IRQ
WFI
DSB
ISB
NOP
system reset
```

`cortex_m3_system_reset()` dùng SCB AIRCR với VECTKEY + SYSRESETREQ.

Core register layout nằm trong:

```text
platform/arch/cortex-m3/include/cortex_m3_registers.h
```

## 15. Device layer skeleton

`platform/device/stm32f103xb/` ban đầu chưa chứa toàn bộ peripheral register map.

`stm32f103xb_memory.h` có bus bases:

```text
FLASH/SRAM
PERIPH
APB1
APB2
AHB
```

Khi thêm peripheral, chỉ thêm register/address/bit cần thiết thay vì copy một vendor header khổng lồ.

## 16. Makefile

Compiler flags chính:

```text
-mcpu=cortex-m3
-mthumb
-std=c11
-Og
-g3
-ffreestanding
-fno-builtin
-ffunction-sections
-fdata-sections
-fno-common
```

Warning:

```text
-Wall
-Wextra
-Wpedantic
-Wshadow
-Wconversion
-Wundef
-Werror=implicit-function-declaration
```

Link:

```text
-nostartfiles
-nostdlib
--gc-sections
custom linker script
-lgcc
```

## 17. Build artifact

```bash
make
```

sinh:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

`firmware.elf` dùng để debug/symbolize; `.bin/.hex` thuận tiện cho flash tool; `.map/.lst` dùng audit layout/disassembly.

## 18. Make targets

```bash
make check-layers
make
make size
make tree
make flash
make erase
make debug-server
make debug
make clean
```

`make debug` kiểm tra GDB command trước khi chạy.

## 19. OpenOCD

Config:

```text
tools/openocd/bluepill_stlink.cfg
```

Dùng:

```tcl
source [find interface/stlink.cfg]
transport select hla_swd
source [find target/stm32f1x.cfg]
reset_config none
adapter speed 1000
```

`reset_config none` phù hợp khi NRST không nối từ probe.

## 20. GDB

`tools/gdb/debug.gdb`:

```text
connect localhost:3333
reset halt
load
reset halt
break main
continue
```

Workflow:

```bash
# terminal 1
make debug-server

# terminal 2
make debug
```

## 21. Cách tạo project mới từ template

Copy template:

```bash
cp -R template my-project
cd my-project
```

Sau đó:

1. đổi `PROJECT` nếu cần,
2. xác định behavior Application,
3. định nghĩa semantic Services,
4. thêm BSP resources,
5. thêm MCAL peripheral,
6. thêm Platform register map,
7. update `system_init()`,
8. update config,
9. chạy layer check,
10. build,
11. viết README/architecture/porting guide.

Xem [`docs/adding_a_module.md`](docs/adding_a_module.md).

## 22. Checklist trước khi coi project mới là hoàn chỉnh

### Architecture

- Application không include hardware layer.
- ISR không gọi upward.
- Board pin chỉ ở BSP.
- Register access chỉ ở MCAL/Platform.

### Runtime

- `.data/.bss` init đúng.
- `main()` hit.
- init failure đi panic.
- IRQ enable sau state init.
- no hidden heap.

### Peripheral

- clock input đúng,
- RCC enable đúng,
- GPIO mode đúng,
- status/error flags clear đúng,
- timeout có bound nếu polling,
- IRQ pending clear trước enable nếu cần.

### Tooling

```bash
make check-layers
make clean
make
make size
```

### Docs

- wiring,
- config,
- expected behavior,
- register flow,
- debug,
- troubleshooting,
- porting notes.

## 23. Tài liệu

- [`docs/architecture.md`](docs/architecture.md) — luật tầng/ownership.
- [`docs/adding_a_module.md`](docs/adding_a_module.md) — quy trình thêm module.
- [`docs/porting_guide.md`](docs/porting_guide.md) — đổi board/MCU.
- [`../examples/README.md`](../examples/README.md) — example thực tế.

## 24. License

Xem [`LICENSE`](LICENSE).
