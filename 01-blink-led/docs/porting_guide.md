# Porting Guide — 01-blink-led

## 1. Porting Goal

Preserve:

```text
Application -> Time Service / Indication Service
```

while replacing only the hardware-specific layers required by the new target.

## 2. Porting to Another Blue Pill with the Same STM32F103

No source changes should be required if:

- onboard LED is still PC13;
- HSE is 8 MHz;
- SWD wiring is unchanged.

Still verify real hardware because clone boards can differ.

## 3. Moving the LED to Another Pin

Change BSP pin mapping and polarity.

MCAL GPIO should remain generic.

Application and Indication Service should not change.

## 4. Changing the System Clock

Update RCC configuration and verify:

- oscillator source;
- PLL multiplier;
- flash wait-state handling;
- APB limits;
- reported system clock.

Board Timebase must receive the actual resulting SYSCLK.

## 5. Changing the Timebase Frequency

If Service APIs are still in milliseconds, preserve a 1 kHz logical timebase or
add explicit conversion.

Do not silently reinterpret ticks as milliseconds after changing tick rate.

## 6. Replacing SysTick with a Timer

Replace Board Timebase/MCAL implementation.

Keep the Time Service API unchanged if it still reports milliseconds.

## 7. Porting to Another STM32F1 MCU

Review:

- RCC differences;
- GPIO register layout;
- memory map;
- startup vectors;
- flash/RAM size;
- SysTick compatibility.

## 8. Porting to Another MCU Family

Keep Application/Services.

Replace BSP, MCAL, Platform Device, startup, linker, and clock logic.

Platform Architecture may remain partly reusable for another Cortex-M.

## 9. Validation Checklist

-  reset reaches `main`;
-  RCC reports expected clock;
-  fallback still works if intended;
-  SysTick runs at 1 kHz;
-  PC13 logical OFF is correct;
-  LED toggles every 500 ms;
-  layer checker passes;
-  debugger attaches reliably.

## 10. Common Mistakes

- moving pin logic into Application;
- forgetting active-low polarity;
- hard-coding a 72 MHz SysTick reload;
- changing SYSCLK without updating peripheral clocks;
- using a blocking delay in Application.

## 11. Target State After Porting

The final dependency should still be:

```text
Application
    |
Services
    |
new BSP
    |
new MCAL
    |
new/updated Platform
```
