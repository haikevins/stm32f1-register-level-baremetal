# Architecture — 07-spi-memory

## 1. Dependency Graph

```text
Application
    |
Memory Service
   / \
  /   \
W25Q64 ECUAL   Board Memory Bus
                    |
                MCAL SPI/GPIO
                    |
              Platform Device
```

Time and Indication Services are separate Application dependencies.

## 2. External-Device Driver Composition

The design separates three concerns:

```text
Application storage policy
W25Q64 command/geometry semantics
STM32 SPI/CS implementation
```

The Memory Service is the composition point.

## 3. Ownership Table

| Concern | Owner |
|---|---|
| destructive self-test | Application |
| logical memory API | Memory Service |
| JEDEC/status/erase/program/read | W25Q64 ECUAL |
| PA4..PA7 | BSP |
| SPI register operation | MCAL SPI |
| GPIO register operation | MCAL GPIO |
| register layout | Platform Device |

## 4. Synchronous Transaction Model

Read/erase/program APIs are synchronous.

They return only after the SPI command and, where required, internal BUSY
polling completes.

This keeps the example simple but means long flash operations occupy thread
mode for a bounded interval.

## 5. Poll Limits vs Timeouts

Unlike the SPL variant of this roadmap, this register-level project uses
iteration poll limits for W25Q64 BUSY completion during startup.

Reason:

```text
global IRQ disabled during system_init()
```

so SysTick-based timeouts cannot safely advance.

## 6. Hardware Transaction Boundary

BSP owns CS and calls MCAL SPI.

ECUAL decides command structure.

```text
ECUAL: which bytes?
BSP: when is CS active?
MCAL: how are bytes clocked?
```

## 7. Read/Write Semantics

Read:

- non-destructive;
- address range validated.

Program:

- Write Enable required;
- page boundary enforced;
- BUSY polled.

Erase:

- sector aligned;
- Write Enable required;
- BUSY polled.

## 8. Error Propagation

Example chain:

```text
MCAL SPI timeout
    |
board transfer false
    |
W25Q64 operation false
    |
Memory Service false
    |
Application diagnostic/error state
```

JEDEC initialization failure propagates to `system_panic()`.

## 9. Concurrency

SPI1 is single-owner and thread-driven in this example.

No SPI IRQ and no shared-bus arbitration are required.

If another device shares SPI1, explicit bus ownership must be added.

## 10. Startup Safety

Startup:

```text
clock
GPIO/CS
SPI
power-on busy delay
JEDEC
self-test
enable global IRQ
```

No step before global IRQ enable depends on SysTick.

## 11. Destructive Boundary

The final 4 KiB sector is reserved for the demo:

```text
0x007FF000 .. 0x007FFFFF
```

Future application data must not overlap that area.

## 12. Extension Strategy

For richer storage:

```text
Application
    |
Storage/Record Service
    |
Memory Service
    |
W25Q64 ECUAL
    |
Board SPI transport
```

Keep page/sector geometry below high-level record policy.
