# Example 09 — hardware validation

**Date:** 2026-11-01
**Scope:** functional CAN TX/RX, *not* extended stress testing.

## Tested arrangement

- RX: STM32F1 device ID `0x412`, 32-KiB Flash, 10-KiB SRAM target limit.
- TX: STM32F1 device ID `0x410`, 64-KiB Flash, using the same conservative 32-KiB/10-KiB linker.
- Each board: MCP2551 breakout, `PB9 → CTX`, `PB8 ← CRX`, 5-V transceiver supply, common GND.
- CANH-to-CANH and CANL-to-CANL; a 120-ohm termination at each bus end.
- Unpowered measured resistance CANH–CANL: **about 60 ohms**.
- Standard data frame, ID `0x123`, DLC 2, nominal 125 kbit/s, 100-ms send period.

## Evidence from hardware logs

| Check | Observed result | Conclusion |
|---|---|---|
| `make NODE=rx`, `make NODE=tx` | GNU Arm GCC compilation/link PASS | Build PASS |
| `make NODE=rx flash` | OpenOCD `Programming Finished`, `Verified OK` | Flash PASS |
| `make NODE=tx flash` | OpenOCD `Programming Finished`, `Verified OK` | Flash PASS |
| Initial MSP from both `.bin` files | `0x20002800` | Matches configured 10-KiB RAM top |
| Receiver LED PC13 | Blinking continuously | RX activity observed |
| GDB RX | `rx_ok=86`, `rx_bad=0`, `rx_overruns=0`, `ESR=0x0` | Functional receive PASS |
| GDB TX | `tx_ok=160`, `tx_failed=0`, `tx_busy=0`, `ESR=0x0` | Functional transmit PASS |

Both GDB snapshots were acquired on separate sessions and at separate times.
The `ESR=0` values reflect a **single sample**, not a lifetime zero-error guarantee.

## What was not verified

- No synchronized, continuous TX-vs-RX packet-count comparison. No claim of 0% loss.
- No 10,000-frame stress test or long-term stability claim.
- No deliberate disconnect/ACK fault-injection test.
- RX mailbox overwrite count is **software mailbox overwrite**, not a hardware FIFO overflow metric.
- The exact MCU part markings and module-specific 5-V/3.3-V electrical thresholds
  should be checked against the actual board and datasheets when reproducing.

**Result:** Basic two-node bxCAN transmit/receive functional hardware test **PASS**.
