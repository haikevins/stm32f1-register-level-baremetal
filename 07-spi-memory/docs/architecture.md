# Kiến trúc — 07-spi-memory

## 1. Dependency graph

```text
Application
   ├────────> Memory Service
   │             ├────────> W25Q64 ECUAL
   │             │             ↑ transport
   │             └────────> Board Memory Bus
   │                              ├─> MCAL SPI
   │                              └─> MCAL GPIO
   │
   ├────────> Time Service → Board Timebase → MCAL SysTick
   └────────> Indication Service → Board LED → MCAL GPIO
```

## 2. Composition của external-device driver

ECUAL và BSP là sibling layers theo dependency checker; ECUAL không include BSP.

Vì vậy Service tạo transport:

```text
Board transfer/select/deselect callbacks
              ↓
         W25Q64 ECUAL
```

Đây là pattern tái sử dụng tốt cho EEPROM, sensor, display hoặc radio driver.

## 3. Ownership table

| Concern | Owner |
|---|---|
| Destructive self-test policy | Application |
| Memory semantic API | Memory Service |
| W25Q64 commands/geometry | ECUAL |
| SPI1/PA4..PA7 mapping | BSP |
| CS behavior | BSP |
| SPI registers | MCAL SPI |
| GPIO registers | MCAL GPIO |
| system/bus clocks | MCAL RCC |
| heartbeat timing | Time Service/SysTick |

## 4. Synchronous transaction model

Service/ECUAL API hiện synchronous:

```text
call erase
  → function không return cho tới khi BUSY clear hoặc poll limit hit
```

Điều này chấp nhận được cho startup self-test nhưng có latency lớn. Production super-loop có thể cần asynchronous state machine.

## 5. Poll limits vs timeouts

W25Q64 busy wait dùng iteration limits vì operation nằm trong init phase khi global IRQ disabled. Đây là architectural constraint của lifecycle hiện tại.

Nếu chuyển erase/program sang runtime thread mode, có thể dùng Time Service để có timeout theo milliseconds.

## 6. Hardware transaction boundary

`board_memory_bus_select/deselect()` định nghĩa transaction ownership. ECUAL bảo đảm:

```text
one command = one or more transfer calls inside one CS-low window
```

MCAL SPI không tự điều khiển CS.

Điều này cho phép bus SPI share với device khác, miễn BSP/service arbitration được thiết kế thêm.

## 7. Read/write semantics

`w25q64_read` là non-destructive nhưng synchronous.

`page_program` và `sector_erase`:

- validate range,
- wait ready,
- WREN + WEL verify,
- issue command,
- wait completion.

## 8. Error propagation

MCAL transfer false
 → BSP false
 → ECUAL false
 → Service false
 → Application increments error

Không có exception/global error manager.

## 9. Concurrency

SPI path không dùng ISR, nên không có concurrent transfer trong example. Nếu thêm task/device khác dùng cùng SPI1, cần bus ownership/arbitration ở tầng phù hợp.

## 10. Startup safety

Board init delay sử dụng busy delay vì global IRQ disabled. Sau đó memory init/self-test cũng không cần SysTick để timeout.

Heartbeat chỉ bắt đầu sau `main()` enable IRQ.

## 11. Destructive boundary

Sector test address là Application config, không hard-code trong ECUAL. W25Q64 driver chỉ cung cấp erase/program primitives.

Đây là separation quan trọng: device driver không quyết định dữ liệu nào được phép xóa.

## 12. Extension strategy

- filesystem/log policy → Service/Application,
- W25Qxx protocol → ECUAL,
- shared SPI bus → BSP/bus service,
- SPI DMA → MCAL,
- alternate MCU → Platform/MCAL/BSP.
