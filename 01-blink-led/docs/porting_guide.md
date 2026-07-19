# Porting Guide

## New application on the same Blue Pill board

Keep `platform/`, `mcal/`, and `bsp/bluepill/`. Replace `app/` and add only the services or
ECU abstraction drivers required by the new product.

## New STM32F103 board

Create a new directory under `bsp/`, for example:

```text
bsp/custom_board/
├── include/
└── src/
```

Map logical resources such as status LED, console, timebase, buttons, and external-device
chip selects there. Do not add board pin numbers to application or services.

Update `CPPFLAGS` and source globs in the Makefile to select the new BSP.

## New STM32F103 memory density

Add a linker script with the correct Flash/RAM sizes and select it in `LDFLAGS`. Do not assume
that a part marked C8 provides 128 KiB Flash.

## New Cortex-M MCU family

Replace or extend:

```text
platform/device/
platform/arch/
mcal/
startup/
linker/
```

Application and portable services should remain unchanged when their public contracts are
preserved.

## Add an external device

An SSD1306, EEPROM, RTC IC, sensor, or transceiver belongs in `ecual/`. Its implementation
uses MCAL/BSP APIs; the application must use it through a service rather than including its
low-level driver directly in a strict closed-layer configuration.
