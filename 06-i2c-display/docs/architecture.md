# Architecture — 06-i2c-display

## 1. Dependency Graph

```text
Application
    |
Display Service
   / \
  /   \
SSD1306 ECUAL   Board Display Bus
                    |
                MCAL I2C/GPIO

Application
    |
Time Service -> Board Timebase -> MCAL SysTick
```

## 2. Why the Service Is the Composition Point

The Service knows both:

- the logical display API;
- the board transport callbacks required to initialize SSD1306.

ECUAL remains independent from BSP.

## 3. Ownership Table

| Concern | Owner |
|---|---|
| screen content | Application |
| logical draw/present API | Display Service |
| framebuffer/controller | SSD1306 ECUAL |
| I2C control byte/address | Board Display Bus |
| I2C protocol registers | MCAL I2C |
| PB6/PB7 | BSP |
| register layout | Platform Device |

## 4. Initialization Timing Nuance

Global IRQ is disabled during `system_init()`.

The OLED power-on delay therefore uses an IRQ-independent busy delay.

After global IRQ is enabled, SysTick provides normal runtime timing.

## 5. I2C Polling Semantics

Each transaction returns success/failure synchronously.

Waits are bounded by `MCAL_I2C_POLL_TIMEOUT_CYCLES`.

No I2C ISR can call upward.

## 6. Framebuffer Ownership

The 1024-byte framebuffer belongs to ECUAL.

Application never sees page/bit layout.

This keeps drawing code independent from I2C transactions.

## 7. Runtime Update Flow

```text
100 ms due
    |
Application updates logical content
    |
Display Service
    |
ECUAL modifies framebuffer
    |
present()
    |
Board bus
    |
MCAL I2C transaction
```

## 8. Failure Model

Initialization failure propagates to `system_panic()`.

Runtime update failure is recorded in Application diagnostics and further
updates stop.

## 9. Layer Boundary Benefits

The same SSD1306 driver can use another board transport.

The same Application-facing display Service can later wrap another controller
with minimal Application changes.

## 10. Extension Notes

Keep:

- graphics/controller logic in ECUAL;
- bus electrical/peripheral logic in BSP/MCAL;
- screen policy in Application.

Avoid leaking raw I2C flags upward.
