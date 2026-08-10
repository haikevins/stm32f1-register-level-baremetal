# Porting Guide — 06-i2c-display

## 1. Changing the OLED Address

Change:

```c
BOARD_DISPLAY_I2C_ADDRESS_7BIT
```

Keep it as a 7-bit address.

## 2. Changing Bus Speed

Update the requested I2C clock.

MCAL recalculates CCR/TRISE from actual PCLK1.

Verify rise time and device support.

## 3. Changing I2C Pins

Update BSP pin mapping.

If moving away from default I2C1 pins, add AFIO remap support as required.

## 4. Changing OLED Controller

Keep the Board Display Bus.

Replace ECUAL and adapt Display Service only if logical drawing semantics
change.

## 5. Changing Resolution

Review:

- framebuffer size;
- page count;
- address window;
- clipping;
- layout constants.

## 6. Adding a Reset Pin

Map reset in BSP and provide a board-level reset operation.

Keep reset polarity/pin details out of ECUAL/Application where possible.

## 7. Changing Power-On Delay

Adjust:

```c
BOARD_DISPLAY_POWER_ON_DELAY_MS
```

The startup delay must remain independent from SysTick while IRQ is disabled.

## 8. Porting to Another MCU

Keep Display Service/SSD1306 ECUAL.

Replace BSP, MCAL I2C/GPIO, and Platform Device.

## 9. Verification Checklist

- [ ] SDA/SCL idle HIGH;
- [ ] correct address;
- [ ] SCL frequency valid;
- [ ] START/address ACK/data/STOP correct;
- [ ] display initialization succeeds;
- [ ] framebuffer update succeeds;
- [ ] error counter remains zero.

## 10. Logic Analyzer Checklist

Capture:

```text
START
address + W
ACK
control byte
payload
STOP
```

Verify `0x00` for command and `0x40` for data.

## 11. Common Pitfalls

- 8-bit address used instead of 7-bit;
- missing pull-ups;
- push-pull instead of open-drain;
- swapped SDA/SCL;
- SysTick-based startup delay before IRQ enable;
- unbounded I2C polling;
- wrong HSI fallback timing.
