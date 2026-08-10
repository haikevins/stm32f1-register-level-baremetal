# Porting Guide — template

Porting nên được làm theo boundary, không sửa tất cả layer cùng lúc. Mục tiêu tốt nhất là giữ Application/Service càng nguyên vẹn càng tốt.

## 1. Trường hợp A — project mới trên cùng Blue Pill

Giữ:

```text
startup/
linker/
platform/
system skeleton
tools/
```

Thêm/đổi:

```text
app/
services/
bsp resources
ecual
mcal modules
config
```

Đây là use case đơn giản nhất.

## 2. Trường hợp B — board khác nhưng vẫn STM32F103C8T6

Tạo BSP mới:

```text
bsp/<board-name>/
```

Thay include path/source selection trong Makefile nếu muốn giữ nhiều BSP.

BSP mới định nghĩa:

- LED,
- button,
- console UART,
- external-device CS,
- sensor bus pins,
- crystal frequency nếu khác.

Platform/MCAL có thể giữ nếu MCU giống.

## 3. Trường hợp C — STM32F103 nhưng memory density khác

Cần ít nhất:

- linker FLASH/RAM lengths,
- có thể vector/device variant,
- startup IRQ list nếu density line khác,
- peripheral availability.

Không assume C8 linker 64K/20K cho mọi part.

## 4. Trường hợp D — STM32F1 part khác

Rà:

```text
platform/device/
startup/
linker/
mcal/
```

Có thể giữ Cortex-M3 architecture layer nếu core giống.

Kiểm tra:

- base address,
- IRQ number,
- peripheral count,
- GPIO ports,
- DMA mapping,
- timer channels,
- clock tree,
- Flash latency rules.

## 5. Trường hợp E — MCU family khác nhưng vẫn Cortex-M

Thay device layer và phần lớn MCAL.

Có thể giữ ý tưởng Architecture primitive, nhưng register như SCB/SysTick/NVIC phụ thuộc Cortex profile/version; không copy mù.

Application/portable Service lý tưởng vẫn giữ.

## 6. Trường hợp F — architecture khác

Thay:

```text
platform/arch/
startup/
linker/
compiler flags
fault model
interrupt primitives
```

Đây là port sâu.

## 7. BSP selection strategy

Nếu một repo chứa nhiều board, không nên rải:

```c
#ifdef BOARD_X
```

khắp MCAL/Application.

Ưu tiên:

- thư mục BSP riêng,
- build variable chọn include/source,
- cùng public board API.

## 8. Clock porting

Clock là nguồn lỗi phổ biến.

Lập bảng:

| Clock | Normal | Fallback | Consumer |
|---|---:|---:|---|
| SYSCLK | ... | ... | core/SysTick |
| PCLK1 | ... | ... | I2C/USART2/TIM |
| PCLK2 | ... | ... | USART1/SPI1/ADC |
| Timer input | ... | ... | PWM/trigger |
| ADC clock | ... | ... | ADC |

Không hard-code tần số vào peripheral driver nếu có thể truyền actual clock.

## 9. GPIO porting

Với mỗi pin:

```text
function
port/pin
input/output/AF/analog
pull/open-drain
active level
speed
remap
```

Pin mapping thuộc BSP.

## 10. Interrupt porting

Checklist:

- vector name,
- IRQ number,
- priority bits,
- pending-clear sequence,
- peripheral source enable,
- shared state,
- strong symbol,
- grouped IRQ behavior.

Một ISR compile được chưa có nghĩa vector mapping đúng.

## 11. DMA porting

DMA mapping thường khác đáng kể giữa MCU.

Xác minh:

```text
request source
controller/channel/stream
data width
direction
increment
circular
IRQ
flag clear
```

Không copy `ADC1 → DMA1 Channel1` sang MCU khác theo thói quen.

## 12. External-device driver portability

ECUAL transport-based driver thường port dễ nhất.

Nếu driver chỉ nhận:

```text
transfer
select
deselect
delay
```

thì đổi MCU chủ yếu thay BSP/MCAL.

Đây là lý do giữ device protocol không phụ thuộc register STM32.

## 13. Linker porting

Cập nhật:

```text
FLASH ORIGIN/LENGTH
RAM ORIGIN/LENGTH
stack reserve
section placement
```

Nếu MCU có nhiều RAM bank/Flash bank, linker cần thiết kế lại, không chỉ đổi LENGTH.

## 14. Startup porting

Vector table phải phù hợp MCU.

Runtime `.data/.bss` copy có thể tái sử dụng nếu memory model tương tự, nhưng linker symbols phải đồng bộ.

## 15. Toolchain flags

Template hiện:

```text
-mcpu=cortex-m3 -mthumb
```

Đổi core phải sửa.

FPU-capable MCU cần quyết định ABI/FPU flags thống nhất compile/link.

## 16. OpenOCD

Target config:

```tcl
source [find target/stm32f1x.cfg]
```

phải đổi khi target family đổi.

Adapter config/reset wiring cũng có thể khác.

## 17. Debug reset strategy

Hiện config:

```tcl
reset_config none
```

phù hợp setup không NRST.

Nếu board/probe có NRST và cần connect-under-reset, có thể thay strategy, nhưng cần test với hardware thực.

## 18. Validation theo tầng

### Startup

- vector ở đúng address,
- MSP đúng,
- `main` hit.

### Clock

- clock register đúng,
- peripheral clock đo/derive đúng.

### GPIO

- level/mode đúng.

### Peripheral

- minimal transaction.

### Service/Application

- behavior end-to-end.

## 19. Automated checks

Luôn chạy:

```bash
make check-layers
make clean
make
make size
```

Sau port lớn, inspect map file.

## 20. Port acceptance checklist

- [ ] Linker đúng memory.
- [ ] Startup/vector đúng MCU.
- [ ] CPU flags đúng.
- [ ] Clock tree đúng.
- [ ] BSP pins đúng.
- [ ] MCAL base/bit đúng.
- [ ] IRQ/DMA mapping đúng.
- [ ] Application không bị hardware detail rò lên.
- [ ] Layer checker pass.
- [ ] Hardware test pass.
- [ ] Docs wiring/config cập nhật.
