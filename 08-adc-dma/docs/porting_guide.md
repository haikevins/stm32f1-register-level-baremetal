# Porting Guide — 08-adc-dma

## 1. Đổi ADC input pin/channel

PA0 = ADC1_IN0 hiện tại.

Để đổi:

1. xác định GPIO có ADC channel mapping,
2. update BSP pin,
3. truyền channel mới vào MCAL ADC init,
4. bảo đảm sample-time register đúng nhóm channel,
5. giữ pin analog mode.

Hiện `board_adc_dma_init()` truyền channel `0U` trực tiếp; nên khi port cần biến nó thành board constant nếu muốn generic hơn.

## 2. Đổi sample rate

Sửa:

```c
BOARD_ADC_SAMPLE_RATE_HZ
```

Timer contract yêu cầu:

```text
BOARD_ADC_TRIGGER_TIMER_TICK_HZ % sample_rate == 0
```

và PSC/ARR phải fit 16 bit.

## 3. Đổi timer trigger

ADC external-trigger selection hiện hỗ trợ TIM3 TRGO duy nhất trong enum/MCAL.

Muốn timer khác:

- thêm trigger enum,
- thêm ADC EXTSEL bit mapping,
- tạo/extend timer trigger MCAL,
- update BSP composition.

## 4. Đổi DMA buffer size

Sửa:

```c
BOARD_ADC_DMA_BUFFER_SAMPLE_COUNT
```

Requirements:

- >= 2,
- even,
- <= 65535.

Block size luôn half buffer theo macro.

Lưu ý RAM tăng ở ít nhất DMA buffer + completed block + Service sample array.

## 5. Đổi DMA channel

ADC1 mapping trên STM32F103 dùng DMA1 Channel 1 trong design hiện tại. MCU khác có DMA request routing khác; cần thay MCAL DMA, IRQ mapping và BSP.

## 6. Đổi ADC clock limit

Config đang dùng 12 MHz dù compile guard chỉ kiểm tra không >14 MHz. Khi port, chọn max phù hợp MCU/accuracy requirement và bảo đảm prescaler set có thể tạo clock <= max.

## 7. Đổi reference voltage

Sửa:

```c
BOARD_ADC_REFERENCE_MV
```

chỉ thay scaling estimate. Nếu cần accuracy, đo/calibrate VDDA hoặc dùng reference channel strategy; không coi macro 3300 là measurement thực.

## 8. Multi-channel ADC

Cần thiết kế lại:

- SQR sequence,
- scan mode,
- sample times từng channel,
- DMA buffer layout,
- ADC Service aggregation.

Không chỉ tăng DMA buffer.

## 9. Đổi IRQ priority

`BOARD_ADC_DMA_IRQ_PRIORITY` hiện = 1. Khi ghép UART/EXTI khác, tạo priority plan toàn hệ thống.

## 10. Refactor ISR ownership

Current code đặt DMA handler tại BSP composite pipeline. Nếu coding standard yêu cầu peripheral ISR nằm MCAL:

- MCAL handler có thể capture DMA events/block-half ID,
- BSP/Service poll event trong thread mode,
- không callback upward từ ISR.

Phải giữ data-race safety khi thay design.

## 11. Validation bằng oscilloscope/debug pin

Để xác minh sample rate chính xác, có thể tạm thời:

- route TIM3 output/trigger ra pin nếu hardware mapping cho phép,
- hoặc toggle debug GPIO ở block event (chỉ trong diagnostic build).

Không nên giữ toggle nặng trong production ISR.

## 12. Validation checklist

- PA0 analog,
- ADC clock hợp lệ,
- calibration hoàn tất,
- TIM3 update 1 kHz,
- DMA CNDTR reload circular,
- HT/TC xen kẽ,
- sequence tăng,
- no DMA error,
- no overrun với normal load,
- LED hysteresis đúng.

## 13. Common pitfalls

- nhầm timer input clock với PCLK1,
- ADC clock quá cao,
- quên ADC calibration,
- sample time không phù hợp source impedance,
- DMA width sai,
- CPAR không trỏ ADC DR,
- CNDTR không đúng,
- xử lý block chậm hơn producer,
- scale mV giả định VDDA chính xác.
