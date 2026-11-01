# Porting Example 09

- **Board:** Default assumes STM32F103C8T6 Blue Pill, MCP2551 module with 5 V VCC, PB8 CAN_RX / PB9 CAN_TX (CAN remap=10) and status LED on active-low PC13.
- **CAN pins:** If using PA11/PA12 instead, change GPIO pin definitions *and* AFIO remap; beware the Blue Pill USB data lines.
- **Clock:** `mcal_can_init()` derives the prescaler from `mcal_rcc_get_apb1_clock_hz()`. A non-integer division for the configured bit rate is treated as an error.
- **Acceptance filters:** Change the demo ID in `board_can.c`, `can_service.c`, and `application_config.h` together (future improvement: make this one config constant).
- **Interrupt:** Handler name is `USB_LP_CAN1_RX0_IRQHandler`, defined in the existing Example 08 startup's vector table. NVIC IRQ number 20.
- **Physical bus:** Two 120-ohm end terminations give about 60 ohms across unpowered CANH/CANL. Check whether any transceiver module already includes the resistor.
- **Logical levels:** MCP2551 uses 5 V; RXD may be at 5 V. Verify chosen MCU pin is 5-V-tolerant and configured properly.
- **Mode:** No silent loopback. Real two-node bus and ACK required.
