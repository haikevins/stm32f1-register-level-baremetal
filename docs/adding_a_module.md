# Adding a Module

## Add an MCAL peripheral

Create:

```text
mcal/include/mcal_<peripheral>.h
mcal/src/mcal_<peripheral>.c
```

Then add the peripheral base address, register structure, and bit definitions under:

```text
platform/device/stm32f103xb/include/
```

MCAL may access registers directly. Upper layers may not.

## Add a board resource

Create:

```text
bsp/bluepill/include/board_<resource>.h
bsp/bluepill/src/board_<resource>.c
```

Map physical pins in `board_pins.h`, implement the resource through MCAL APIs, and call its init
function from `board_init()`.

## Add an external device

Create its low-level driver in `ecual/`. The implementation uses MCAL or BSP APIs. Product logic
should normally access the device through a Service API.

## Add a service

Create:

```text
services/include/<name>_service.h
services/src/<name>_service.c
```

The service exposes product-independent operations and may use BSP/ECUAL APIs. Add its init call to
`system/system_init.c` before `application_init()`.

## Add application behavior

Use `app/src/application.c` or add application state-machine modules. Application must remain
non-blocking and must not include BSP, MCAL, STM32, or Cortex-M3 headers.
