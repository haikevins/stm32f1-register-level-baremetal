# Porting Guide

## New application on the same board

Keep `startup/`, `linker/`, `platform/`, and the reusable lower-layer modules. Replace or extend
`app/` and only add the Services, BSP, ECUAL, and MCAL modules needed by the project.

## New STM32F103 board

Create another BSP directory and select it in `CPPFLAGS` and `C_SOURCES`. Keep physical pin numbers
inside the BSP rather than Application or Services.

## New memory density

Add a linker script with the correct Flash and SRAM sizes and select it in `LDFLAGS`.

## New MCU family

Replace or extend:

```text
platform/device/
platform/arch/
mcal/
startup/
linker/
```

Application and portable Services should remain unchanged when their public contracts are kept.
