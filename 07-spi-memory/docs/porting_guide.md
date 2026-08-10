# Porting Guide — 07-spi-memory

## 1. Changing SPI/CS Pins on STM32F103

Update BSP mapping:

- SCK;
- MISO;
- MOSI;
- CS.

If alternate-function remap is required, add AFIO support.

Keep W25Q64 ECUAL unchanged.

## 2. Moving to SPI2

Add/verify:

- SPI2 base address;
- RCC clock bit;
- APB1 clock input;
- GPIO mapping;
- MCAL instance support.

The W25Q64 driver should remain unchanged.

## 3. Changing SPI Frequency

Change:

```c
BOARD_MEMORY_SPI_MAX_HZ
```

MCAL selects a prescaler that does not exceed the requested limit.

Verify SCK under both normal and fallback clocks.

## 4. Using a Different Flash Capacity

Review together:

- total size;
- JEDEC validation;
- address width;
- page size;
- sector size;
- erase command set.

Do not change only one capacity constant.

## 5. Changing the Test Sector

Choose a valid aligned 4 KiB sector.

Document that it is destructive and verify it does not overlap real data.

## 6. Multi-Page Programming

Split writes at page boundaries.

Do not remove the page-boundary protection from the low-level page-program API.

## 7. Runtime Asynchronous Operation

If erase/program latency is too long for thread-mode blocking, convert the
Memory Service into a state machine:

```text
start
poll status in service_process()
publish completion
```

Keep raw SPI/W25Q64 details below Application.

## 8. Shared SPI Bus

Add a bus-ownership/arbitration mechanism.

Each device keeps its own CS.

If devices require different SPI modes/speeds, reconfigure the bus safely while
no device is selected.

## 9. Porting to Another MCU

Keep:

```text
Memory Service
W25Q64 ECUAL
```

Replace BSP, MCAL SPI/GPIO, and Platform Device.

## 10. Validation Checklist

-  CS idle HIGH;
-  no CS glitch during GPIO initialization;
-  SPI mode 0;
-  SCK within configured maximum;
-  JEDEC ID correct;
-  WEL sets;
-  BUSY clears;
-  erase/program/read pass;
-  destructive sector is correct;
-  layer checker passes.

## 11. Common Pitfalls

- D0/D1 reversed;
- 5 V power;
- wrong SPI mode;
- CS toggled mid-command;
- missing dummy clocks;
- page boundary crossed;
- no Write Enable;
- SysTick-based wait before IRQ enable;
- test sector overlaps important data.
