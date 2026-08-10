# Kiến trúc — 03-uart-polling

## 1. Dependency graph

```text
Application
    ↓
Serial Service
    ↓
Board UART
    ↓
MCAL USART
    ├─ MCAL GPIO
    └─ MCAL RCC
    ↓
Platform STM32F103
```

## 2. Trách nhiệm

| Module | Trách nhiệm |
|---|---|
| Application | Greeting + echo policy + debug counters |
| Serial Service | API byte-oriented portable |
| Board UART | USART instance, pin mapping, baud config |
| MCAL USART | Register setup, RX/TX polling, error capture |
| MCAL GPIO | PA9/PA10 mode |
| MCAL RCC | Clock setup, APB2 clock |
| Platform | USART1 base/register/bit definitions |
| System | Composition/init/super-loop |

## 3. Public contract

Service API:

```c
bool serial_service_try_read_byte(uint8_t *byte);
bool serial_service_try_write_byte(uint8_t byte);
uint32_t serial_service_take_error_flags(void);
```

`try_*` có contract "không chờ". Điều này ảnh hưởng trực tiếp đến Application state design.

## 4. Polling ownership

MCAL sở hữu hardware readiness:

```text
RXNE
TXE
PE/FE/NE/ORE
```

Application chỉ nhận `bool` và portable error flags.

## 5. Error handoff

Hardware flags:

```text
USART_SR
  ↓
portable_error_flags()
  ↓
g_error_flags[instance]
  ↓
mcal_usart_take_error_flags()
  ↓
Board UART
  ↓
Serial Service
  ↓
Application counters
```

`take` semantics read-and-clear accumulated software state.

## 6. Không có interrupt concurrency

USART path không có shared RX/TX state giữa ISR và thread mode. Vì vậy architecture đơn giản hơn Example 04:

- không NVIC,
- không ring index,
- không critical section TX start,
- không overflow counter phần mềm.

## 7. Timing dependency

Throughput phụ thuộc tần suất super-loop gọi `application_process()`. Nếu thêm task blocking vào Application, UART polling dễ bị overrun.

Do đó "non-blocking upper-layer design" là yêu cầu hệ thống, không chỉ style.

## 8. Initialization dependency

Board cần tính clock trước UART:

```text
RCC setup
 → get APB2 clock
 → board_uart_init
 → mcal_usart_init
```

Sai clock input dẫn tới sai baud dù USART register sequence đúng.

## 9. Layer boundary

Đổi USART1 sang USART2 lý tưởng chỉ tác động:

- BSP,
- MCAL instance mapping,
- platform register/clock bit,
- clock source APB tương ứng.

Application/Serial Service không đổi.

## 10. Failure path

`board_uart_init()` false:

```text
board_init false
 → system_init false
 → system_panic
```

Configuration error được fail sớm thay vì chạy UART với divider invalid.
