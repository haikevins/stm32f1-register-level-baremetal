# Kiến trúc — 04-uart-interrupt-ring-buffer

## 1. Dependency graph

```text
Application
    ↓
Serial Service
    ↓
Board UART
    ↓
MCAL USART
    ├─ USART1 register access
    ├─ NVIC setup
    ├─ RX ring
    ├─ TX ring
    ├─ error/overflow counters
    └─ USART1_IRQHandler
    ↓
Platform device/architecture
```

## 2. State ownership

| State | Writer chính | Reader chính |
|---|---|---|
| RX head | ISR | thread observes |
| RX tail | thread | ISR observes |
| TX head | thread | ISR observes |
| TX tail | ISR | thread observes |
| USART error flags | ISR | thread take/clear |
| RX overflow counter | ISR | thread take/clear |
| Greeting/echo state | Application | Application |

## 3. SPSC reasoning

RX và TX ring gần với single-producer/single-consumer:

- RX producer = ISR, consumer = thread,
- TX producer = thread, consumer = ISR.

Điều này giảm số critical section cần thiết. Tuy nhiên TX "kick" (`TXEIE`) là register state có thể bị cả ISR/thread thay đổi nên enqueue dùng critical section.

## 4. RX flow

```text
Wire → USART shift register → DR/RXNE
                          ↓ IRQ
                    USART1_IRQHandler
                          ↓
                     RX ring
                          ↓
              serial_service_try_read
                          ↓
                     Application
```

## 5. TX flow

```text
Application
    ↓ try_write
TX ring
    ↓ enable TXEIE
USART1_IRQHandler
    ↓
DR → shift register → wire
```

## 6. Interrupt rules

ISR được phép:

- snapshot `SR`,
- read/write `DR`,
- advance ring index,
- set counters,
- enable/disable TXEIE.

ISR không được:

- call Service/Application,
- format text,
- echo,
- debounce,
- allocate.

## 7. Error semantics

Hardware receive error và software queue overflow là hai domain khác nhau, vì vậy có hai API/state riêng. Điều này làm debug tốt hơn so với một generic "UART failed" flag.

## 8. API backpressure

`try_write_byte()` trả false khi TX ring full. Đây là backpressure rõ ràng. Application quyết định giữ pending byte thay vì MCAL block.

`try_read_byte()` false khi RX ring empty.

## 9. Buffer capacity

Size phải power-of-two vì index wrap dùng mask. Một slot để trống:

```text
effective capacity = configured size - 1
```

Nếu đổi implementation sang count-based ring, contract/capacity cần document lại.

## 10. Initialization safety

Global IRQ disable trong `main` trước `system_init()`. MCAL có thể configure NVIC/USART an toàn mà handler chưa chạy giữa chừng. Sau khi toàn bộ state reset và Application init hoàn tất, global IRQ mới enable.

## 11. Clock boundary

BSP truyền PCLK2 thực tế cho MCAL. MCAL không biết board crystal. Đây là separation quan trọng để HSI fallback vẫn tạo baud divider phù hợp.

## 12. Extension points

- Parser protocol: Service/Application, không ISR.
- Framing buffer: Service.
- DMA UART: MCAL/BSP mới, giữ Serial Service contract nếu phù hợp.
- Flow control CTS/RTS: BSP + MCAL.
- Multiple USART: mở rộng instance mapping trong MCAL/BSP.
