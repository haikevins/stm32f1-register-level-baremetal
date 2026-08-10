# 07-spi-memory — SPI1 + W25Q64 NOR Flash

## 1. Learning Objectives

This example adds a register-level SPI bus and a W25Q64-compatible NOR flash
driver.

You will learn:

- SPI1 mode 0 configuration;
- PA4 software chip select;
- PA5/PA6/PA7 SCK/MISO/MOSI mapping;
- register-level SPI baud-prescaler selection;
- bounded TXE/RXNE/BSY polling;
- W25Q64 JEDEC identification;
- Write Enable and Status Register-1;
- BUSY polling;
- 4 KiB Sector Erase;
- Page Program restrictions;
- read-back verification;
- ECUAL transport composition;
- startup-safe polling while global IRQ is disabled.

## 2. Wiring

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

Important mapping:

```text
D1 = DO = MISO = PA6
D0 = DI = MOSI = PA7
```

Use 3.3 V power and logic.

## 3. Expected Behavior

At startup the firmware:

1. configures the board clock and SPI bus;
2. waits for the memory power rail to settle;
3. reads the W25Q64 JEDEC ID;
4. validates the manufacturer/capacity;
5. erases the last 4 KiB sector;
6. programs a 32-byte test pattern;
7. reads the 32 bytes back;
8. verifies them byte-for-byte.

If the test passes:

```text
PC13 toggles every 500 ms
```

If erase/program/read-back verification fails after the device is identified:

```text
PC13 remains ON
```

If memory initialization/JEDEC validation fails, `system_init()` fails and the
firmware enters `system_panic()`.

## 4. W25Q64 Geometry in the Driver

```text
capacity: 8 MiB
address range: 0x000000 .. 0x7FFFFF
page size: 256 bytes
sector size: 4096 bytes
```

Demo sector:

```text
0x007FF000 .. 0x007FFFFF
```

**Warning:** this sector is erased once on every reset.

## 5. JEDEC ID

Command:

```text
0x9F
```

The driver reads three bytes:

```text
manufacturer
memory type
capacity
```

Expected manufacturer/capacity:

```text
manufacturer = 0xEF
capacity     = 0x17
```

A common W25Q64 result is:

```text
EF 40 17
```

The memory-type byte is recorded for debugging.

## 6. SPI Configuration

SPI1 runs as:

```text
master
full duplex
8-bit frames
MSB first
CPOL = 0
CPHA = 0
software NSS
```

This is SPI mode 0.

The MCAL enables SPI1 through APB2 and configures CR1 directly.

## 7. SPI Clock Selection

Configuration:

```c
#define BOARD_MEMORY_SPI_MAX_HZ (5000000UL)
```

MCAL chooses the smallest power-of-two SPI divider that does not exceed the
requested maximum.

At normal clock:

```text
PCLK2 = 72 MHz
divider = /16
SPI = 4.5 MHz
```

At 8 MHz HSI fallback:

```text
PCLK2 = 8 MHz
divider = /2
SPI = 4 MHz
```

## 8. Chip Select

PA4 is a normal push-pull GPIO controlled by BSP.

Idle state:

```text
CS = HIGH
```

Transaction:

```text
CS LOW
command/address/data
CS HIGH
```

The GPIO configuration path preloads the desired output level before switching
the pin into output mode, avoiding an unwanted active-low CS glitch during
initialization.

## 9. SPI Transfer Primitive

For each byte:

```text
wait TXE
    |
write DR
    |
wait RXNE
    |
read DR
```

For receive-only bytes, the MCAL transmits dummy `0xFF`.

After the final byte:

```text
wait BSY == 0
```

Every status wait is bounded by:

```c
MCAL_SPI_POLL_TIMEOUT_CYCLES
```

## 10. W25Q64 Command Set Used by the Demo

| Operation | Command |
|---|---:|
| Write Enable | `0x06` |
| Read Status Register-1 | `0x05` |
| Read Data | `0x03` |
| Page Program | `0x02` |
| 4 KiB Sector Erase | `0x20` |
| JEDEC ID | `0x9F` |

## 11. Status Register-1

### BUSY

Bit 0 indicates an internal program/erase operation is still active.

The ECUAL repeatedly reads Status Register-1 until BUSY clears or the configured
poll limit is reached.

### WEL

Bit 1 is the Write Enable Latch.

Before erase/program:

```text
Write Enable command
    |
read Status Register-1
    |
WEL set?
```

If WEL is not set, the operation fails.

## 12. Bounded Polling

Because global interrupts are disabled during `system_init()`, the W25Q64 driver
does **not** depend on SysTick for erase/program completion.

It uses bounded poll counts:

```c
#define W25Q64_READY_POLL_LIMIT          (10000UL)
#define W25Q64_PAGE_PROGRAM_POLL_LIMIT   (100000UL)
#define W25Q64_SECTOR_ERASE_POLL_LIMIT   (1000000UL)
```

This keeps initialization safe before global IRQ enable.

The 10 ms memory power-on settling delay also uses an IRQ-independent MCAL busy
delay.

## 13. Page Program Rules

The driver validates:

- non-null data;
- length > 0;
- length <= 256;
- address range valid;
- write does not cross a 256-byte page boundary.

Conceptually:

```text
wait ready
    |
Write Enable
    |
verify WEL
    |
CS LOW
0x02 + 24-bit address + data
CS HIGH
    |
poll BUSY
```

## 14. Sector Erase Rules

The requested address is aligned down to a 4 KiB sector boundary.

Sequence:

```text
wait ready
Write Enable
verify WEL
send 0x20 + 24-bit address
poll BUSY
```

The demo deliberately erases the final sector.

## 15. Read Transaction

```text
CS LOW
0x03
A23..A16
A15..A8
A7..A0
dummy clocks -> data bytes
CS HIGH
```

The ECUAL validates that the requested range stays inside the 8 MiB device.

## 16. ECUAL Transport

The W25Q64 ECUAL receives a generic transport:

```text
transfer()
select()
deselect()
```

The Memory Service composes those callbacks from BSP.

Therefore the W25Q64 driver does not include BSP/MCAL/Platform headers.

## 17. Application Self-Test

Configured test:

```c
#define MEMORY_DEMO_TEST_SECTOR_ADDRESS (0x007FF000UL)
#define MEMORY_DEMO_TEST_LENGTH         (32U)
```

Sequence:

```text
erase sector
    |
program 32 bytes
    |
read 32 bytes
    |
compare every byte
```

On success:

```text
application_memory_test_sequence = 1
application_memory_test_passed = true
```

## 18. Debug Globals

```gdb
p/x application_memory_manufacturer_id
p/x application_memory_type_id
p/x application_memory_capacity_id

p/x application_memory_test_address
p application_memory_test_sequence
p application_memory_error_count

p application_memory_erase_ok
p application_memory_program_ok
p application_memory_verify_ok
p application_memory_test_passed

p/x application_memory_first_mismatch_index
p/x application_memory_readback_first_byte
p/x application_memory_readback_last_byte
```

Successful read-back pattern:

```text
first byte = 0x53
last byte  = 0xA5
```

## 19. LED Indication

Pass:

```text
PC13 toggles every 500 ms
```

Application-level test failure:

```text
PC13 steady ON
```

Initialization failure:

```text
system_panic()
```

## 20. Interrupt Policy

SPI1 interrupts are not used.

`SPI1_IRQHandler()` remains the weak default startup handler.

SysTick is enabled only after `system_init()`; W25Q64 startup operations are
therefore intentionally independent from SysTick.

## 21. Architecture

```text
Application
    |
Memory Service
   / \
  /   \
W25Q64 ECUAL   Board Memory Bus
                    |
                MCAL SPI + GPIO
                    |
              Platform Device
```

The Application also uses Time/Indication Services for the post-test heartbeat.

## Build, Flash, and Debug

```bash
make check-layers
make clean
make
make flash
```

```bash
# Terminal 1
make debug-server

# Terminal 2
make debug
```

## 22. Test Procedure

1. Wire the module exactly as documented.
2. Flash firmware.
3. Inspect JEDEC ID.
4. Confirm manufacturer `0xEF` and capacity `0x17`.
5. Confirm erase/program/verify flags are true.
6. Confirm error count is zero.
7. Confirm first/last read-back bytes.
8. Confirm PC13 heartbeat.
9. Optionally capture SPI with a logic analyzer.

## 23. JEDEC Troubleshooting

### `00 00 00`

Check:

- MISO/D1 connected to PA6;
- module power;
- common ground;
- MISO not shorted low.

### `FF FF FF`

Check:

- CS actually goes LOW;
- MISO is not open/floating;
- correct module power;
- SPI pin mapping.

### ID Is Correct but Program/Erase Fails

Basic SPI wiring is likely correct.

Focus on:

- Write Enable;
- WEL;
- BUSY polling;
- sector address;
- page boundary;
- poll limits.

## 24. Logic Analyzer

For JEDEC:

```text
CS LOW
9F
dummy -> EF
dummy -> type
dummy -> 17
CS HIGH
```

Verify SPI mode 0 and continuous CS across one command transaction.

## 25. Wear/Endurance Note

The demo erases the same sector at every reset.

That is acceptable for controlled learning, but it is not a production storage
policy.

Avoid unnecessary repeated resets and do not store important data in the test
sector.

## 26. Extension Exercises

- multi-page programming;
- larger reads;
- device-ID abstraction;
- record storage;
- CRC-protected records;
- shared SPI bus;
- asynchronous erase/program state machine.

## 27. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
