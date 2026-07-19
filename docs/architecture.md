# Layered Architecture Rules

## Runtime layers

```text
Application
    |
    v
Services
    |
    v
BSP / ECU Abstraction
    |
    v
MCAL
    |
    v
STM32F103 Device / Cortex-M3 Architecture
    |
    v
Hardware
```

`system/` is the composition root. `startup/`, `linker/`, `config/`, and `tools/` are
infrastructure rather than product layers.

## Allowed dependencies

| Layer | May depend on | Must not depend on |
|---|---|---|
| Application | Services, portable Common types | BSP, ECUAL, MCAL, STM32/Cortex headers |
| Services | BSP, ECUAL, Common | Application, raw registers |
| BSP/ECUAL | MCAL, Common | Services, Application |
| MCAL | Device, Architecture, Common | BSP, Services, Application |
| Device/Architecture | Standard integer headers, compiler primitives | Every upper layer |
| Common | Standard portable headers | Hardware-specific modules |

## Interrupt ownership

An interrupt handler belongs to the lowest module that owns the peripheral. It may acknowledge
flags and store data into a module-owned static buffer. It must not include an Application header,
run product state machines, allocate dynamic memory, or callback upward.

## Public and private APIs

Public headers belong in `<layer>/include/`. Private headers stay beside their source file or in a
private include directory that is not globally exported.

## Composition root

Initialization order is connected in `system/system_init.c`:

```text
MCAL prerequisites -> BSP/ECUAL -> Services -> Application
```

This file may include multiple layers, but it must not contain product behavior.
