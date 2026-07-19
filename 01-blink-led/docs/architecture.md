# Architecture

## 1. Layer map

```text
+--------------------------------------------------+
| Application                                      |
| State machine, product behavior                  |
+--------------------------+-----------------------+
                           |
                           v
+--------------------------------------------------+
| Services                                         |
| Time, indication, events, protocols, diagnostics |
+--------------------------+-----------------------+
                           |
                           v
+--------------------------------------------------+
| BSP / ECU Abstraction                            |
| Board resources and external devices             |
+--------------------------+-----------------------+
                           |
                           v
+--------------------------------------------------+
| MCAL                                             |
| GPIO, RCC, SysTick, UART, SPI, I2C, CAN, DMA     |
+--------------------------+-----------------------+
                           |
                           v
+--------------------------------------------------+
| Device / Architecture                            |
| STM32F103 register map and Cortex-M3 core         |
+--------------------------+-----------------------+
                           |
                           v
+--------------------------------------------------+
| Hardware                                         |
+--------------------------------------------------+
```

## 2. Allowed dependencies

| Module | May depend on | Must not depend on |
|---|---|---|
| Application | Services, common pure types | BSP, ECUAL, MCAL, STM32/Cortex headers |
| Services | BSP, ECUAL, Common | Application, raw registers |
| BSP/ECUAL | MCAL, Common | Services, Application |
| MCAL | Device, Architecture, Common | BSP, Services, Application |
| Device/Architecture | `stdint`, compiler primitives | Every upper layer |
| Common | Standard integer/types headers | Hardware-specific modules |

## 3. Composition root exception

`system/` is not an application layer. It owns startup ordering and the super-loop, so it may
include public initialization APIs from several layers. Business behavior must not be placed
in `system/`.

## 4. Interrupt rule

An interrupt handler belongs to the lowest module that owns the peripheral. It may:

- Read or acknowledge peripheral flags.
- Move data into an MCAL-owned static buffer.
- Increment an MCAL-owned counter.

It must not:

- Include an application header.
- Run protocol parsing or state-machine logic.
- Call a service/application callback upward.
- Block or allocate dynamic memory.

The upper layer polls an MCAL/BSP API during normal thread-mode execution.

## 5. Public/private headers

A public header belongs in `<layer>/include/`. Internal register mapping and private state
should remain in the source directory or use a `_private.h` header that is not added to the
global include path.

## 6. Common is a foundation library

`common/` may contain ring buffers, fixed queues, CRC, bit helpers, and generic data types.
It must remain portable and must not include STM32 headers.

## 7. Build-time infrastructure

The following directories are not runtime layers:

- `startup/`
- `linker/`
- `tools/`
- `config/`

They may select or connect runtime modules but must not contain product behavior.
