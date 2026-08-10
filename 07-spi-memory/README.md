# 07-spi-memory

Register-level bare-metal W25Q64 SPI flash example for the STM32F103C8T6
Blue Pill.

## Wiring

```text
STM32F103C8T6       W25Q64
--------------------------------
3.3V        ------  VCC
GND         ------  GND
PA4         ------  CS
PA5         ------  CLK
PA6         ------  D1 / DO / MISO
PA7         ------  D0 / DI / MOSI
```

Use 3.3 V logic and a common ground.

## Behavior

On every reset the firmware:

1. reads the three-byte JEDEC ID,
2. verifies Winbond manufacturer ID and 64-Mbit capacity code,
3. erases the last 4 KiB sector at `0x007FF000`,
4. programs a 32-byte test pattern,
5. reads the 32 bytes back,
6. verifies every byte.

This is a destructive demo: the final 4 KiB sector is erased on every reset.

When the test passes, the onboard PC13 LED toggles every 500 ms. A steady ON
LED indicates erase/program/read-back verification failure after device
identification.

## SPI

- peripheral: SPI1
- CS: PA4, software controlled
- SCK: PA5
- MISO: PA6
- MOSI: PA7
- mode: 0 (CPOL=0, CPHA=0)
- frame: 8 bit, MSB first
- polling, no SPI interrupt
- maximum configured clock: 5 MHz
- normal 72 MHz PCLK2: prescaler /16 = 4.5 MHz
- 8 MHz HSI fallback: prescaler /2 = 4 MHz

## Architecture

```text
Application
    |
    v
Memory Service
   / \
  /   \
W25Q64 ECUAL     Board Memory Bus
                    |
                    v
               MCAL SPI / GPIO
                    |
                    v
             STM32F103 registers
```

The service composes the board SPI transport with the W25Q64 ECUAL. The
device driver does not include BSP or STM32 register headers.

## Debug

```gdb
p/x application_memory_manufacturer_id
p/x application_memory_type_id
p/x application_memory_capacity_id

p/x application_memory_test_address
p application_memory_erase_ok
p application_memory_program_ok
p application_memory_verify_ok
p application_memory_test_passed
p application_memory_error_count
p/x application_memory_first_mismatch_index
p/x application_memory_readback_first_byte
p/x application_memory_readback_last_byte
```

A common W25Q64 result is JEDEC ID `EF 40 17`.

## Build

```sh
make check-layers
make clean
make
make flash
```

`system_idle()` intentionally uses `NOP`, not `WFI`, to keep SWD attach
predictable with ST-Link adapters that do not expose NRST.
