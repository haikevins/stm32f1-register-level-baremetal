# 09 - CAN Bus - bxCAN TX/RX Between Two STM32F103 Nodes

> A register-level STM32F103 bxCAN example using two Blue Pill boards and MCP2551 transceivers. One firmware tree builds into independent TX and RX nodes; it uses no HAL, LL, SPL, RTOS, or dynamic allocation.

[Examples index](../README.md) · [Architecture](docs/architecture.md) · [Porting guide](docs/porting_guide.md) · [Hardware validation](docs/hardware_validation.md)

## Table of contents

- [Purpose and expected behavior](#purpose-and-expected-behavior)
- [Hardware and wiring](#hardware-and-wiring)
- [Configuration](#configuration)
- [Runtime flow](#runtime-flow)
- [Register-level mechanism](#register-level-mechanism)
- [Initialization and failure propagation](#initialization-and-failure-propagation)
- [Concurrency and ownership](#concurrency-and-ownership)
- [Observability and debugging](#observability-and-debugging)
- [Design trade-offs and limitations](#design-trade-offs-and-limitations)
- [Build and run](#build-and-run)
- [References](#references)

## Purpose and expected behavior

Example 09 introduces Classical CAN communication without a vendor peripheral library. Two STM32F103 nodes share a 125-kbit/s CAN bus through MCP2551 transceivers. The same source tree is compiled with `NODE=tx` for the transmitter or `NODE=rx` for the receiver.

The expected behavior is deliberately observable:

1. The **TX node** requests a standard 11-bit CAN data frame with identifier `0x123`, DLC 2, and bytes `[counter, 0x11]` every 100 ms. The 8-bit counter wraps from 255 to 0.
2. The **RX node** accepts matching standard data frames through bxCAN filter bank 0 and FIFO0, then publishes the latest received frame from an interrupt to thread mode.
3. The CAN service validates the frame and toggles the PC13 LED on each accepted frame. The application exposes transmit/receive counters for GDB inspection.

A powered receiver does **not** send application data in this example. In normal CAN mode, its controller can still acknowledge valid frames on the bus. This distinction matters because CAN transmission success depends on acknowledgment from another active node.

## Hardware and wiring

**Required setup:** two STM32F103 Blue Pill boards, two MCP2551 transceiver modules (with `VCC`, `GND`, `CTX`, `CRX`, `CANH`, `CANL`), a common ground, and 120-ohm termination at each physical end of the CAN bus. One ST-Link can be moved between boards for programming and debugging.

Wire **each** STM32 board to its own module:

| STM32F103 Blue Pill | MCP2551 module | Purpose |
| --- | --- | --- |
| PB9 | CTX | Remapped bxCAN transmit |
| PB8 | CRX | Remapped bxCAN receive; verify module output voltage |
| 5 V | VCC | Regulated 5-V transceiver supply |
| GND | GND | Shared ground reference |
| PC13 | Onboard LED | Receive indication on RX board |

Connect the two modules together:

| Connection | Requirement |
| --- | --- |
| CANH ↔ CANH | High bus line |
| CANL ↔ CANL | Low bus line |
| GND ↔ GND | Common ground between both nodes |
| CANH ↔ CANL at each bus end | One 120-ohm termination resistor per end, unless already fitted |

With **all power disconnected**, two 120-ohm terminators in parallel should read approximately **60 ohms** between CANH and CANL. This checks the termination network, not whether frames are actually received. Do not fit extra termination if the modules already provide it.

MCP2551 needs a regulated **5-V supply**, not the STM32 3.3-V output. Check each breakout board's decoupling and verify its `CRX` output is electrically compatible with PB8 under the STM32 datasheet conditions. Using PB8/PB9 for CAN avoids the PA11/PA12 USB D-/D+ pin conflict on Blue Pill boards powered by Micro USB.

## Configuration

The TX and RX roles are selected **at build time** by `NODE`, rather than by changing source files.

| Item | Configuration |
| --- | --- |
| Target | STM32F103 (Cortex-M3), Blue Pill wiring |
| `NODE=tx` | `APPLICATION_CAN_ROLE=1U` |
| `NODE=rx` | `APPLICATION_CAN_ROLE=2U` |
| CAN pins | PB8 = CAN_RX, PB9 = CAN_TX (AFIO remap) |
| Nominal bit rate | 125 kbit/s |
| Frame | Standard 11-bit data ID `0x123`, DLC 2 |
| Payload | `[counter, 0x11]` |
| TX interval | 100 ms, scheduled from a 1-ms SysTick time base |
| CAN timing | 16 TQ, BS1 = 13, BS2 = 2, SJW = 1; sample point 87.5% |
| Normal APB1 clock | 36 MHz; prescaler 18 |
| HSI fallback APB1 clock | 8 MHz; prescaler 4 |
| TX policy | Normal mode, NART enabled (no automatic retransmission), ABOM enabled |
| RX filter | Bank 0, 32-bit mask, exact standard data ID `0x123`, FIFO0 |
| RAM / Flash linker limits | 10 KiB RAM / 32 KiB Flash for both roles |
| Initial main stack pointer | `0x20002800` |

The conservative memory limits support both the 32-KiB and 64-KiB STM32F1 devices used in hardware testing; they are intentionally smaller than the 64-KiB Flash / 20-KiB RAM settings in some earlier examples. CAN bit timing is derived from the active APB1 clock rather than assuming one fixed clock source.

## Runtime flow

Both roles use the repository's cooperative super-loop. Periodic work and application-level frame processing run in thread mode; receiving a CAN frame begins with a hardware interrupt.

```text
                           system_init()
                                 |
                       application_process()
                                 |
                    can_service_process()
                         /             \
          TX completion polling     RX mailbox consumption
                   |                       |
          100-ms timer due?          validate frame
                   |                       |
       submit ID 0x123 via TX0       count valid / invalid
                   |                       |
          bxCAN transmits                toggle PC13
                   |
         other CAN node ACKs

CAN FIFO0 --> RX0 IRQ --> latest-frame mailbox --> thread-mode service
SysTick --> 1-ms counter --> periodic TX scheduling
```

`can_service_send()` queues a frame to bxCAN TX mailbox 0 when it is available; `can_service_process()` polls for completion on subsequent iterations. The RX node has the same CAN peripheral initialization and receive interrupt, but application code does not schedule transmissions in `NODE=rx` mode.

## Register-level mechanism

The implementation programs STM32F103 peripheral registers directly, with hardware access isolated below the application and service layers.

| Peripheral / register group | Role in Example 09 |
| --- | --- |
| RCC APB2 / APB1 | Enable AFIO and CAN1 clocks; reset CAN1 before configuration |
| AFIO MAPR | Remap CAN_RX/CAN_TX to PB8/PB9 |
| GPIO configuration | PB8 floating input; PB9 alternate-function push-pull output |
| CAN MCR / MSR | Enter/leave initialization mode with bounded waits; configure NART/ABOM |
| CAN BTR | Set 125-kbit/s nominal bit timing from PCLK1 |
| CAN FMR / filter bank 0 | Exact 11-bit ID and data-frame filtering to FIFO0 |
| CAN IER / NVIC | Enable FIFO0 message-pending interrupt |
| CAN TI0 registers / TSR | Submit a frame via transmit mailbox 0 and poll completion |
| CAN RI0 registers / RF0R | Read received identifier, DLC and payload; release FIFO0 entry |
| CAN ESR | Expose error status for inspection through GDB |

The configured 16 time quanta and prescaler give `36 MHz / (18 × 16) = 125 kbit/s` on the normal APB1 clock path, or `8 MHz / (4 × 16) = 125 kbit/s` on the fallback path. The configuration is also covered by `tests/test_bit_timing.py`.

The receive interrupt `USB_LP_CAN1_RX0_IRQHandler` processes at most three hardware FIFO0 entries per invocation, keeping ISR work bounded. The application deliberately does not parse frames or toggle the LED inside the ISR.

## Initialization and failure propagation

Startup and memory initialization follow the common register-level example skeleton: vector table and reset handler establish the C runtime before `main()` calls `system_init()`.

The initialization path is:

```text
Reset_Handler --> .data/.bss runtime init --> main()
    --> system_init()
        --> board_init() (clock, GPIO, SysTick, CAN)
        --> time_service_init()
        --> indication_service_init()
        --> can_service_init()
        --> application_init()
    --> application_process() / system_idle()
```

Board initialization returns failure when a required peripheral configuration step fails. bxCAN initialization includes bounded waits for initialization-mode acknowledgment. An unsuccessful `system_init()` follows the repository's panic path instead of silently continuing with an unusable CAN controller.

Only after successful initialization does the normal super-loop process CAN events and periodic transmission. CAN transmission failures are observed through service counters rather than being treated as fatal startup errors.

## Concurrency and ownership

The dependency direction matches the other register-level examples:

| Layer | Responsibility |
| --- | --- |
| `app/` | TX/RX demo policy, 100-ms scheduling and GDB-visible snapshots |
| `services/` | CAN payload formation/validation, statistics, time and LED indication policy |
| `bsp/bluepill/` | PB8/PB9 binding, CAN RX0 ISR and one-frame handoff mailbox |
| `mcal/` | CAN, GPIO, NVIC, RCC, SysTick and interrupt-control primitives |
| `platform/` | Cortex-M3 and STM32F103 register definitions |
| `system/`, `startup/`, `linker/` | Composition, reset path, memory and main loop |

RX interrupt context owns writes to a **single latest-frame mailbox**. Thread mode reads and clears it through `board_can_take_received()` using a short PRIMASK-protected critical section. If another frame arrives before the previous one is consumed, the mailbox is overwritten and an overrun counter increases. This is intentional for a learning example, **not** a lossless receive queue.

Application and service statistics are updated in thread mode. The TX request/completion state is polled without a blocking send loop. SysTick supplies the 1-ms timing reference used to request a new TX frame approximately every 100 ms.

## Observability and debugging

The board's **PC13 LED** changes state after each frame that passes the application-level checks (ID, DLC and marker byte). GDB variables expose both roles' statistics:

| TX node | RX node | Shared observation |
| --- | --- | --- |
| `application_can_tx_ok` | `application_can_rx_ok` | `application_can_esr` |
| `application_can_tx_failed` | `application_can_rx_bad` | — |
| `application_can_tx_busy` | `application_can_rx_overruns` | — |
| — | `application_can_last_counter` | — |

To inspect a running node, connect ST-Link to that board and run OpenOCD and GDB from two terminals:

```bash
# Terminal 1 (from 09-can-bus/)
make NODE=rx debug-server

# Terminal 2 (after Terminal 1 starts)
gdb-multiarch -q build/rx/firmware.elf
```

Inside GDB:

```gdb
target extended-remote localhost:3333
monitor reset halt
continue
```

Interrupt GDB with **Ctrl+C** after the board has been running, then inspect the appropriate role:

```gdb
# RX board
p application_can_rx_ok
p application_can_rx_bad
p application_can_rx_overruns
p application_can_last_counter
p/x application_can_esr

# TX board: connect ST-Link to TX and use build/tx/firmware.elf
p application_can_tx_ok
p application_can_tx_failed
p application_can_tx_busy
p/x application_can_esr
```

For the hardware-functional test performed **2026-11-01**, both boards were flashed and verified with ST-Link/OpenOCD. With both boards unpowered, the bus measured approximately 60 ohms across its two 120-ohm terminators. The following independent GDB snapshots were recorded:

| Observation | Result |
| --- | --- |
| TX completed | `tx_ok = 160` |
| TX failed / busy | `tx_failed = 0`, `tx_busy = 0` |
| RX valid frames | `rx_ok = 86` |
| RX invalid / overwritten mailbox | `rx_bad = 0`, `rx_overruns = 0` |
| CAN ESR at inspection | `0x00000000` on each board |
| RX indicator | PC13 blinking continuously |

The TX and RX snapshots were collected in **different runs**; they cannot be subtracted to estimate loss rate. Full scope and caveats are recorded in [Hardware validation](docs/hardware_validation.md).

When troubleshooting missing frames, first verify the regulated 5-V transceiver supplies, common ground, PB8/PB9 wiring and AFIO remap, CANH/CANL polarity and two correct terminations, then inspect CAN ESR and transmitter completion status.

## Design trade-offs and limitations

- **Small teaching scope:** standard 11-bit CAN data frames only; no extended frames, CANopen, ISO-TP or fragmentation.
- **Latest-frame RX mailbox:** receive frames may be overwritten under load. `application_can_rx_overruns` describes mailbox overwrites, not every hardware FIFO overrun or every lost frame.
- **Bounded ISR, cooperative service:** FIFO0 is drained at most three entries per ISR. Application validation and LED updates remain in thread mode.
- **NART mode:** automatic retransmission is disabled. A missing ACK can produce a failed transmission; that fault scenario has not been validated on hardware here.
- **Limited error telemetry:** the application exposes the sampled CAN ESR register and simple counters; it is not a complete CAN fault recovery or logging subsystem.
- **Validated scope:** the hardware test demonstrated normal short-run TX/RX functionality, not zero frame loss, long-duration reliability, 10,000-frame stress results, or reset/bus-off recovery.
- **Hardware assumptions:** MCP2551 needs 5 V, electrical compatibility of module RX output must be confirmed, and the two CAN ends must have appropriate termination.
- **Memory model:** 32-KiB Flash / 10-KiB RAM allocation is intentional for the smaller tested device; it does not imply other STM32F103 variants have identical available memory.

## Build and run

Requirements: GNU Arm embedded toolchain (`arm-none-eabi-gcc`, binutils), GNU Make, Python 3, OpenOCD, ST-Link V2 and optionally `gdb-multiarch`.

From the repository root:

```bash
cd 09-can-bus

# Build and validate both independent firmware roles
make NODE=tx check-layers
make NODE=tx
make NODE=rx check-layers
make NODE=rx

# Optional bit-timing check
python3 tests/test_bit_timing.py
```

Artifacts are in `build/tx/` and `build/rx/` (`firmware.elf`, `firmware.bin`, `firmware.hex`, listing and linker map). The Makefile uses GCC for Cortex-M3 and directly owned startup/linker scripts. Both roles include non-executable `.note.GNU-stack` metadata at assembly time; the GNU-stack warning present in the earlier baseline was resolved without changing either `.bin` image.

Flash **one board at a time** when sharing one ST-Link. Keep the other node powered for the actual CAN communication test:

```bash
# ST-Link connected to TX board
make NODE=tx flash

# Move ST-Link to RX board
make NODE=rx flash
```

Makefile targets follow the repository's standard vocabulary (`check-layers`, `clean`, `size`, `tree`, `flash`, `erase`, `debug-server`, `debug`), with the CAN-specific `NODE=tx`/`NODE=rx` selector. Use the corresponding role for all build/debug commands; the default `NODE` is `tx`.

## References

- STMicroelectronics, **RM0008** — STM32F1 reference manual (bxCAN, RCC, AFIO, NVIC and FIFO/mailbox registers).
- STMicroelectronics, **PM0056** — STM32F10xxx Cortex-M3 programming manual.
- Microchip, **MCP2551 datasheet** — physical layer, 5-V supply and transceiver electrical characteristics.
- [Example 09 architecture notes](docs/architecture.md).
- [Example 09 hardware validation](docs/hardware_validation.md).
- [Example 09 porting guide](docs/porting_guide.md).
