# 08-adc-dma — TIM3 trigger + ADC1 + DMA1 circular buffer

Example này tạo một pipeline lấy mẫu analog tự động:

```text
TIM3 update event
    ↓ TRGO
ADC1 channel 0 / PA0
    ↓ DMA request
DMA1 Channel 1 circular buffer
    ↓ HT / TC interrupt
block 32 samples
    ↓
ADC Service
    ↓
average / minimum / maximum / millivolts
    ↓
Application
    ↓
PC13 LED hysteresis
```

Mục tiêu là tách **sample timing**, **data movement** và **signal processing** thành các phần độc lập. CPU không cần poll ADC cho từng conversion.

## 1. Mục tiêu học tập

Example minh họa:

- GPIO analog mode,
- ADC clock prescaler,
- ADC1 reset/power-up/calibration,
- regular sequence một channel,
- sample-time register,
- external trigger TIM3 TRGO,
- timer update master-mode output,
- DMA1 Channel 1 peripheral-to-memory,
- halfword transfers,
- circular buffer,
- half-transfer/full-transfer/error interrupt,
- block handoff ISR → thread mode,
- critical section khi lấy completed block,
- averaging/min/max ở Service,
- chuyển raw ADC sang mV,
- LED hysteresis ở Application,
- phân biệt hardware pipeline với processing policy.

## 2. Hardware

Có thể dùng biến trở:

```text
3.3 V ---- potentiometer ---- GND
                  |
                  +---- PA0 / ADC1_IN0
```

Giới hạn tín hiệu:

```text
GND <= PA0 <= VDDA
```

Không đưa điện áp âm hoặc cao hơn rail analog của MCU.

LED status dùng PC13 active-low.

## 3. Compile-time configuration

`config/board_config.h`:

```c
#define BOARD_HSE_FREQUENCY_HZ                  (8000000UL)
#define BOARD_TARGET_CLOCK_HZ                    (72000000UL)

#define BOARD_ADC_REFERENCE_MV                   (3300UL)
#define BOARD_ADC_MAX_RAW_VALUE                  (4095UL)
#define BOARD_ADC_MAX_CLOCK_HZ                   (12000000UL)
#define BOARD_ADC_SAMPLE_RATE_HZ                 (1000UL)
#define BOARD_ADC_TRIGGER_TIMER_TICK_HZ          (1000000UL)

#define BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT        (64U)
#define BOARD_ADC_DMA_BLOCK_SAMPLE_COUNT         \
    (BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT / 2U)

#define BOARD_ADC_CALIBRATION_TIMEOUT_ITERATIONS (1000000UL)
#define BOARD_ADC_DMA_IRQ_PRIORITY               (1U)
```

Application thresholds:

```c
#define APPLICATION_ADC_LED_ON_THRESHOLD_MV  (1800U)
#define APPLICATION_ADC_LED_OFF_THRESHOLD_MV (1500U)
```

## 4. Data-rate model

Sample rate:

```text
1000 samples/s
```

DMA buffer:

```text
64 × uint16_t = 128 byte
```

Mỗi half-buffer:

```text
32 samples = 64 byte
```

Block period:

```text
32 samples / 1000 samples/s = 32 ms
```

Do đó Service thường có một block mới khoảng mỗi 32 ms nếu super-loop chạy đủ nhanh.

## 5. Timer trigger

TIM3 được dùng như trigger generator, không dùng TIM3 interrupt.

Target timer tick:

```text
1 MHz
```

Target trigger:

```text
1 kHz
```

Ở SYSCLK 72 MHz:

```text
PCLK1 = 36 MHz
TIM3 input clock = 72 MHz
PSC divider = 72
PSC = 71
ARR = 999
```

TIM3 master-mode selection:

```text
CR2.MMS = Update
```

Mỗi update event phát TRGO tới ADC.

Trong HSI fallback 8 MHz:

```text
TIM3 input clock = 8 MHz
PSC = 7
ARR = 999
```

Sample rate logic vẫn 1 kHz.

## 6. ADC input configuration

PA0 được cấu hình:

```text
GPIO mode = analog input
```

Điều này tránh digital input path không cần thiết và đúng với ADC input use case.

ADC sequence:

```text
Regular sequence length: 1
SQ1 = channel 0
Data alignment: reset/default right-aligned trong register profile hiện tại
```

Sample time channel 0:

```text
55.5 ADC cycles
```

MCAL ghi sample code vào `SMPR2` cho channel 0..9.

## 7. ADC clock selection

ADC clock được lấy từ PCLK2 qua prescaler:

```text
/2, /4, /6, /8
```

MCAL chọn divisor đầu tiên sao cho:

```text
ADC clock <= BOARD_ADC_MAX_CLOCK_HZ
```

Với PCLK2 72 MHz:

```text
/2 = 36 MHz > 12 MHz
/4 = 18 MHz > 12 MHz
/6 = 12 MHz <= 12 MHz
```

Nên ADCPRE = /6.

Với HSI 8 MHz:

```text
/2 = 4 MHz <= 12 MHz
```

## 8. ADC reset/power/calibration sequence

`mcal_adc1_init_regular_channel()`:

1. chọn ADC clock,
2. enable ADC1 clock,
3. pulse ADC1 reset,
4. clear CR1/CR2/sample/sequence registers,
5. set SQ1 = channel,
6. set sample time,
7. set `ADON`,
8. NOP stabilization loop,
9. set `RSTCAL`, chờ bit clear,
10. set `CAL`, chờ bit clear,
11. configure:
    - `ADON`,
    - `DMA`,
    - external trigger selection = TIM3 TRGO,
    - `EXTTRIG`.

Calibration waits dùng bounded iteration count.

## 9. DMA mapping

ADC1 data register address được MCAL ADC expose dưới dạng `uintptr_t`; DMA MCAL dùng làm `CPAR`.

DMA1 Channel 1 config:

```text
Peripheral → Memory
Peripheral increment: disabled
Memory increment: enabled
Peripheral width: 16 bit
Memory width: 16 bit
Circular: enabled
Priority: high
HT interrupt: enabled
TC interrupt: enabled
TE interrupt: enabled
```

`CNDTR = 64`.

DMA memory target:

```c
static volatile uint16_t g_dma_buffer[64];
```

## 10. DMA interrupt events

DMA flags được MCAL chuyển thành portable event mask:

```text
MCAL_DMA_EVENT_HALF_TRANSFER
MCAL_DMA_EVENT_TRANSFER_COMPLETE
MCAL_DMA_EVENT_TRANSFER_ERROR
```

`mcal_dma_adc1_channel1_take_irq_events()`:

- đọc DMA ISR,
- map flag,
- ghi IFCR để clear cờ,
- trả event mask.

## 11. IRQ handler và block publishing

Trong implementation hiện tại, `DMA1_Channel1_IRQHandler()` nằm trong `bsp/bluepill/src/board_adc_dma.c` vì BSP đang sở hữu composite ADC+DMA pipeline và completed-block state.

Handler:

```text
TE:
  error_count++

HT:
  copy dma_buffer[0..31] → completed_block

TC:
  copy dma_buffer[32..63] → completed_block
```

Nếu một block cũ vẫn chưa được thread mode lấy:

```text
g_block_ready == true
```

khi block mới publish:

```text
g_overrun_count++
```

Sau đó completed block mới ghi đè block cũ.

ISR không tính average/mV và không điều khiển LED.

## 12. Vì sao copy block trong ISR

DMA circular buffer tiếp tục bị hardware ghi. BSP copy half vừa hoàn tất sang `g_completed_block[32]` để Service có snapshot ổn định sau khi DMA chuyển sang half kia.

Trade-off:

- ISR dài hơn kiểu chỉ enqueue pointer/event,
- nhưng Service không bị race với DMA đang wrap/ghi.

Với block 32 halfword, chi phí copy được giới hạn và rõ ràng trong demo.

## 13. Thread-mode handoff

`board_adc_dma_take_sample_block()`:

1. validate destination capacity,
2. save PRIMASK + disable IRQ,
3. kiểm tra `g_block_ready`,
4. copy completed block sang Service buffer,
5. clear `g_block_ready`,
6. restore PRIMASK.

Critical section bảo vệ completed snapshot/state khỏi ISR publish đồng thời.

## 14. ADC Service processing

Service nhận 32 mẫu và tính:

```text
minimum_raw
maximum_raw
sum
average_raw
millivolts
sequence
```

Average có rounding:

```text
average = (sum + sample_count/2) / sample_count
```

Voltage estimate:

```text
millivolts =
    (average_raw × BOARD_ADC_REFERENCE_MV
     + BOARD_ADC_MAX_RAW_VALUE/2)
    / BOARD_ADC_MAX_RAW_VALUE
```

Với reference assumption 3300 mV:

```text
raw 0    ≈ 0 mV
raw 2048 ≈ 1650 mV
raw 4095 ≈ 3300 mV
```

Đây là estimate dựa trên macro reference, không đo VDDA thực tế.

## 15. Application hysteresis

Application dùng hai threshold:

```text
ON  >= 1800 mV
OFF <= 1500 mV
```

Vùng 1500..1800 mV giữ state trước.

Lợi ích:

- tránh LED chatter khi signal/noise dao động quanh một threshold duy nhất,
- minh họa stateful decision ở Application.

## 16. Debug globals

```gdb
p application_adc_average_raw
p application_adc_minimum_raw
p application_adc_maximum_raw
p application_adc_millivolts
p application_adc_sequence
p application_adc_dma_overruns
p application_adc_dma_errors
```

Normal:

```text
sequence tăng liên tục
dma_errors = 0
dma_overruns = 0
```

Nếu halt debugger đủ lâu trong lúc acquisition chạy, overrun có thể tăng sau resume vì upper layer không consume block.

## 17. Interrupt/symbol expectations

Strong:

```text
DMA1_Channel1_IRQHandler
```

Weak/default:

```text
TIM3_IRQHandler
ADC1_2_IRQHandler
```

TIM3 chỉ phát TRGO; ADC chỉ phát DMA request.

## 18. Initialization flow

```text
system_init
  ├─ board_init
  │   ├─ RCC clock
  │   ├─ board_led_init
  │   └─ board_adc_dma_init
  │       ├─ PA0 analog
  │       ├─ DMA1 Ch1 configure
  │       ├─ TIM3 trigger configure
  │       ├─ ADC1 configure/calibrate
  │       ├─ NVIC DMA1_Ch1 enable
  │       ├─ DMA start
  │       └─ TIM3 start
  ├─ adc_service_init
  ├─ indication_service_init
  └─ application_init
```

Global IRQ enable sau toàn bộ sequence.

## 19. Kiến trúc

```text
Application
   ├─> ADC Service
   │      ↓
   │   Board ADC DMA
   │      ├─> MCAL ADC
   │      ├─> MCAL DMA
   │      ├─> MCAL Timer Trigger
   │      ├─> MCAL NVIC
   │      └─> MCAL IRQ
   │
   └─> Indication Service
          ↓
       Board LED
          ↓
       MCAL GPIO
```


## Build, flash và debug

Yêu cầu công cụ:

```text
arm-none-eabi-gcc
arm-none-eabi-objcopy
arm-none-eabi-objdump
arm-none-eabi-size
GNU Make
Python 3
OpenOCD
arm-none-eabi-gdb hoặc gdb-multiarch
```

Build:

```bash
make check-layers
make clean
make
```

Các artifact chính:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

Flash:

```bash
make flash
```

Debug bằng hai terminal:

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

OpenOCD config dùng SWD, `reset_config none` và adapter speed 1000 kHz.


DMA breakpoint:

```gdb
break DMA1_Channel1_IRQHandler
continue
```

Sau khi xác nhận IRQ, bỏ breakpoint để acquisition chạy bình thường rồi halt và inspect globals.


## 20. Test procedure

1. Nối biến trở.
2. Flash firmware.
3. Xoay về gần GND → mV gần 0, LED OFF.
4. Xoay lên > 1.8 V → LED ON.
5. Giảm xuống 1.6 V → LED vẫn giữ ON.
6. Giảm < 1.5 V → LED OFF.
7. Inspect average/min/max/mV.
8. Xác nhận sequence tăng và error/overrun 0.

## 21. Troubleshooting

### Raw luôn 0

Kiểm tra:

- wiper thật sự nối PA0,
- PA0 analog mode,
- TIM3 CEN,
- TRGO MMS update,
- ADC EXTTRIG/EXTSEL,
- DMA EN/CNDTR.

### Raw luôn 4095

Kiểm tra PA0 bị kéo 3.3 V hoặc analog source/wiring.

### DMA IRQ không chạy

Kiểm tra:

- DMA1 clock,
- Channel 1 mapping,
- HTIE/TCIE,
- NVIC enable IRQ 11,
- ADC DMA bit,
- timer trigger/conversion thật sự xảy ra.

### `dma_overruns` tăng

Thread mode không consume block kịp. Kiểm tra blocking code/debug halt. Có thể tăng buffer/queue, nhưng root cause là consumer latency.

### mV không đúng đồng hồ đo

`BOARD_ADC_REFERENCE_MV = 3300` chỉ là assumption. VDDA thực tế có thể khác; production measurement cần calibration/reference strategy.

## 22. Bài tập mở rộng

- nhiều ADC channel scan,
- DMA buffer lớn hơn,
- moving average/IIR,
- RMS calculation,
- continuous streaming UART,
- trigger frequency runtime,
- analog watchdog,
- calibrated VDDA,
- lock-free block queue,
- double-buffer ownership không copy ISR.

## 23. Tài liệu liên quan

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
