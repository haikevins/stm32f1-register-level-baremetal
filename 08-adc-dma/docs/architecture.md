# Kiến trúc — 08-adc-dma

## 1. Hardware data path

```text
TIM3 update
   ↓ TRGO
ADC1 channel 0
   ↓ DR + DMA request
DMA1 Channel 1
   ↓ circular memory
g_dma_buffer[64]
   ↓ HT / TC IRQ
g_completed_block[32]
   ↓ thread-mode take
ADC Service
   ↓
adc_measurement_t
   ↓
Application
```

## 2. Software dependency graph

```text
Application
   ├────────────> ADC Service
   │                 ↓
   │             Board ADC DMA
   │              /   |   |   \
   │       MCAL ADC  DMA Timer NVIC/IRQ
   │                 ↓
   │              Platform
   │
   └────────────> Indication Service
                         ↓
                     Board LED
                         ↓
                     MCAL GPIO
```

## 3. Ownership table

| State/resource | Owner |
|---|---|
| TIM3 trigger config | MCAL Timer Trigger |
| ADC registers/calibration | MCAL ADC |
| DMA registers/flags | MCAL DMA |
| DMA IRQ enable | MCAL NVIC |
| `g_dma_buffer` | Board ADC DMA pipeline |
| `g_completed_block` | Board ADC DMA pipeline |
| block-ready/overrun/error | Board ADC DMA |
| sample processing | ADC Service |
| threshold/hysteresis | Application |

## 4. ISR placement trong code hiện tại

Khác một số example khác nơi ISR nằm trực tiếp MCAL, handler DMA hiện ở BSP composite module:

```text
bsp/bluepill/src/board_adc_dma.c
```

MCAL vẫn sở hữu raw DMA register/flag handling; BSP sở hữu mapping "DMA event nào tương ứng half-buffer nào" và completed block.

Tài liệu phải phản ánh implementation này khi refactor/port.

## 5. Concurrency zones

Có hai vùng shared ISR/thread:

```text
g_block_ready
g_completed_block
g_overrun_count
g_error_count
```

Thread-mode `take_sample_block` disable IRQ ngắn trong lúc copy/clear state.

DMA hardware đồng thời ghi `g_dma_buffer`, nhưng ISR chỉ copy half đã hoàn tất. Circular DMA chuyển tiếp sang half còn lại.

## 6. Overrun semantics

Overrun ở đây không phải ADC hardware overrun flag. Nó có nghĩa:

```text
producer đã publish block mới
trong khi consumer chưa take block cũ
```

Counter là indicator cho system-level processing latency.

## 7. Sampling determinism

Sample interval do TIM3 hardware quyết định, không phụ thuộc super-loop execution time. Đây là ưu điểm chính so với polling ADC từ Application.

Thread-mode processing có thể jitter nhưng sample timestamp spacing vẫn dựa trên trigger hardware.

## 8. Service decoupling

ADC Service nhận một snapshot array và output semantic struct:

```c
adc_measurement_t
```

Application không biết DMA block size khi chỉ consume measurement, ngoài debug timing behavior.

## 9. Error propagation

DMA transfer error:

```text
DMA flag
 → MCAL event
 → BSP error_count
 → ADC Service getter
 → Application debug global
```

ADC calibration/config failure xảy ra init và propagate qua `board_init` → panic.

## 10. Memory use

Core acquisition buffers:

```text
DMA buffer:       64 × 2 = 128 byte
completed block:  32 × 2 = 64 byte
Service buffer:   32 × 2 = 64 byte
```

Ngoài các state/counters khác. Đây là trade-off rõ ràng giữa snapshot simplicity và RAM.

## 11. Extension options

### Multi-channel scan

Cần update:

- ADC sequence length/SQR registers,
- DMA interleaved interpretation,
- Service block parser.

### No-copy ping-pong

Có thể tránh ISR copy bằng ownership protocol trên DMA halves, nhưng cần xử lý trường hợp hardware wrap trước consumer.

### Queue nhiều block

Thêm static block queue để absorb latency, nhưng vẫn phải định nghĩa overflow policy.

## 12. Boundary rule

Không đưa raw `ADC1->DR`, DMA ISR flags hoặc TIM3 register lên Application. Nếu Application cần sample rate metadata, expose qua semantic config/API thay vì register access.
