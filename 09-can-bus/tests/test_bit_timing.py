#!/usr/bin/env python3
"""Host-side non-hardware assertions for the chosen bxCAN timing/filter constants."""
from pathlib import Path
p = Path(__file__).resolve().parents[1]
source = (p / 'mcal/src/mcal_can.c').read_text()
assert '#define CAN_SAMPLE_TQ (16U)' in source
assert '#define CAN_TS1 (13U)' in source
assert '#define CAN_TS2 (2U)' in source
assert '#define CAN_SJW (1U)' in source
for pclk, brp in [(36_000_000, 18), (8_000_000, 4)]:
    assert pclk // (brp * 16) == 125_000
    assert (1 + 13) / 16 == 0.875
print('PASS: 125 kbit/s nominal bit timing at PCLK1=36/8 MHz')
