# Kiến trúc — 06-i2c-display

## 1. Dependency graph

```text
Application
   ├───────────────> Time Service
   │                    ↓
   │                Board Timebase
   │                    ↓
   │                MCAL SysTick
   │
   └───────────────> Display Service
                       ├────────> SSD1306 ECUAL
                       │             ↑ transport callbacks
                       └────────> Board Display Bus
                                      ↓
                                 MCAL I2C + GPIO
                                      ↓
                                    Platform
```

## 2. Vì sao Service là composition point

Theo layer checker:

```text
ECUAL không được include BSP
BSP không được include ECUAL
Service được phép phụ thuộc cả hai
```

Do đó `display_service_init()` tạo `ssd1306_transport_t` từ BSP callbacks và truyền xuống ECUAL.

Đây là dependency inversion đơn giản mà không cần heap/object framework.

## 3. Ownership table

| Concern | Owner |
|---|---|
| UI content/progress | Application |
| Display-facing semantic API | Display Service |
| SSD1306 protocol/commands/framebuffer | ECUAL |
| OLED I2C control byte/address | BSP Display Bus |
| PB6/PB7/I2C1 mapping | BSP |
| START/ADDR/TXE/BTF/STOP | MCAL I2C |
| 100 ms UI schedule | Time Service/Application |
| 100 ms power-on settle | BSP/MCAL busy delay |

## 4. Initialization timing nuance

Global IRQ bị disable trước `system_init()`. Vì display init nằm trong `system_init()`, power-on delay không thể dựa vào SysTick ISR.

Architecture tách:

```text
init delay: busy NOP MCAL delay
runtime schedule: SysTick Time Service
```

Điều này cần nhớ khi thêm bất kỳ peripheral init nào đòi delay trước global IRQ enable.

## 5. I2C polling semantics

Không có I2C ISR. MCAL polling có bounded loop để tránh treo vĩnh viễn nếu flag không đến.

Upper layer chỉ nhận `bool`, không biết SR1/SR2.

## 6. Framebuffer ownership

`ssd1306.c` sở hữu static framebuffer 1024 byte. Application không giữ raw pixel buffer và chỉ gọi draw methods.

Điều này giữ rendering state trong device abstraction.

## 7. Runtime update flow

```text
Application periodic due
 → advance progress
 → clear framebuffer
 → draw text/progress
 → display_service_present
 → ssd1306_update
 → command addressing
 → I2C data stream 1024 bytes
```

## 8. Failure model

Transport returns `false` → ECUAL return false → Service return false → Application ghi error và disable further updates.

Init failure propagate tới panic.

## 9. Layer boundary benefits

Đổi OLED sang device khác:

- Application có thể giữ logic nếu Display Service contract giữ,
- ECUAL thay driver,
- BSP I2C có thể tái sử dụng.

Đổi I2C1 sang I2C2:

- ECUAL không đổi,
- Application không đổi.

## 10. Extension notes

Nếu thêm asynchronous I2C:

- transport contract hiện synchronous bool; cần thiết kế lại state/callback/event,
- không gọi Application callback từ ISR,
- có thể Service poll transaction state.
