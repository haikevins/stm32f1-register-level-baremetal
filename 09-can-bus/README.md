# 09 - bxCAN (STM32F103 Blue Pill + MCP2551)

> Register-level CAN 2.0A Standard Data Frame demo; two Blue Pill nodes, one source tree, TX polling and RX0 interrupt. No HAL/LL/SPL, no heap.

[Examples index](../README.md) · [Architecture](docs/architecture.md) · [Porting](docs/porting_guide.md) · [Hardware validation](docs/hardware_validation.md)

## Wiring (both nodes)

| Blue Pill | MCP2551 module | Note |
|---|---|---|
| PB9 | CTX | CAN TX, remapped |
| PB8 | CRX | CAN RX, remapped; check breakout output voltage |
| 5V | VCC | MCP2551 needs a regulated 5 V supply |
| GND | GND | common reference |
| CANH module 1 | CANH module 2 | bus high |
| CANL module 1 | CANL module 2 | bus low |
| GND node 1 | GND node 2 | common ground |

Place one 120 ohm resistor between CANH/CANL at **each bus end**, only if the module has no onboard termination. With both nodes unpowered, the measured bus termination should be about **60 ohms**. A 60-ohm bus measurement verifies termination only; it does **not** verify power, polarity or data communication.

Breakout modules may include decoupling; check the specific hardware. Verify VCC/GND at both modules (about 5 V) and check CRX output compatibility with PB8 before running. PB8 is a 5-V-tolerant digital pin under the device's specified conditions. If powering the Blue Pill from Micro USB, PB8/PB9 avoids the PA11/PA12 USB D-/D+ conflict. Do not power MCP2551 from the 3.3 V pin.

## Runtime

| Setting | Value |
|---|---|
| `NODE=tx` | Send standard ID `0x123` every 100 ms |
| `NODE=rx` | Receive ID `0x123` (data frames) by FIFO0 IRQ, toggle PC13 |
| DLC | 2 |
| Data | `[counter, 0x11]`, counter wraps at 255 |
| Nominal bitrate | 125 kbit/s |
| Time quanta | 16; TS1=13, TS2=2, SJW=1, sample point=87.5% |
| Normal clock | PCLK1=36 MHz, BRP=18 |
| HSI fallback | PCLK1=8 MHz, BRP=4 |
| CAN mode | Normal, automatic retransmission disabled (NART), ABOM enabled |
| Filter | 32-bit mask bank 0, exact standard data ID `0x123` |

The firmware **does not transmit automatically from the RX node**. bxCAN nevertheless acknowledges any valid CAN frame in normal mode.

## Build and flash

```bash
cd 09-can-bus
make NODE=tx check-layers
make NODE=tx
make NODE=rx check-layers
make NODE=rx
```

**Flash one board at a time** with one ST-Link (PB8/PB9/5V/GND/CANH/CANL connected as described):

```bash
# Connect ST-Link to transmitter board, then:
make NODE=tx flash
# Disconnect ST-Link from transmitter, connect to receiver board:
make NODE=rx flash
```

Both ELF files are independent: `build/tx/firmware.elf` and `build/rx/firmware.elf`. The projects use the existing startup, linker, OpenOCD scripts, clock fallback and layer checker from the series. **For the tested 32-KiB and 64-KiB devices, the current linker script conservatively allocates 32 KiB Flash and 10 KiB RAM**, with initial MSP `0x20002800` for both roles.

**Startup sequencing:** Both boards must be powered. A CAN transmitter cannot report TX success without at least one node ACKing a valid frame. With NART set, missing ACK causes failed transmissions rather than unbounded hardware retries.

## GDB observability

Break into your running firmware and inspect:

```gdb
p application_can_tx_ok
p application_can_tx_failed
p application_can_tx_busy
p application_can_rx_ok
p application_can_rx_bad
p application_can_rx_overruns
p/x application_can_esr
p application_can_last_counter
```

Expected: TX board `application_can_tx_ok` increases when RX board is connected and powered. RX board `application_can_rx_ok` increases approximately 10/s and PC13 toggles; `application_can_last_counter` updates. If nothing arrives, examine 5 V supply, grounds, PB8/PB9 remap, CANH/CANL wiring, termination, and ESR (LEC, TEC, REC, Bus-Off).

## Design and limitations

- ISR drains no more than 3 CAN FIFO0 frames per invocation and publishes only the latest frame to a static mailbox. Overwrites are counted; **not** a lossless CAN receive queue.
- A 1-ms SysTick tracks demo timing. The service never blocks on transmitting; TX completion is polled in thread mode.
- Filter accepts only Standard ID `0x123`, data frames; no CANopen, ISO-TP, extended frames or fragmentation.
- Error status is sampled in `CAN_ESR`. The application exports counters; diagnosis by GPIO/logic analyzer and on-target validation are still required.
- Firmware has been **built and flashed on two physical STM32F1 boards** using GNU Arm and OpenOCD. Basic TX/RX functional validation passed. See [hardware validation](docs/hardware_validation.md). Continuous 10,000-frame/stress, missing-ACK and reset-recovery testing were **not** performed.

## Verified baseline (2026-11-01)

Tested with two STM32F1 boards (OpenOCD identified 32-KiB low-density RX and
64-KiB medium-density TX devices), two MCP2551 modules, two 120-ohm
termination resistors and a common ground. Unpowered bus resistance: about 60 ohms.

- **Build/layer checks:** TX and RX PASS (GNU Arm GCC on Ubuntu).
- **Flash/verify/reset:** both boards PASS (ST-Link V2, OpenOCD).
- **RX GDB snapshot:** `rx_ok=86`, `rx_bad=0`, `rx_overruns=0`, `ESR=0x0`; PC13 blink observed.
- **TX GDB snapshot:** `tx_ok=160`, `tx_failed=0`, `tx_busy=0`, `ESR=0x0`.
- **Not demonstrated:** loss rate, 10,000-frame stress test, ACK-disconnect fault test or
  long-term CAN error recovery. TX and RX counts are snapshots from **different** runs;
  they must not be compared to infer missing frames.

For evidence, interpretation and scope see [hardware validation](docs/hardware_validation.md).

## References

- ST RM0008 (bxCAN registers, CAN remap and NVIC)
- Microchip MCP2551 datasheet (5-V supply and CAN physical layer)
