# 01-blink-led — GPIO Output + SysTick + Non-Blocking Super-Loop

## 1. Learning Objectives

This first example introduces the architecture used throughout the repository.

You will learn how to:

- configure the Blue Pill onboard LED through register-level MCAL;
- hide active-low electrical behavior behind a logical Service;
- build a 1 ms SysTick timebase;
- schedule periodic work without a blocking delay;
- handle 72 MHz HSE/PLL startup with 8 MHz HSI fallback;
- keep Application independent from GPIO and SysTick registers.

## 2. Expected Result

The onboard PC13 LED changes state every 500 ms.

Because the LED is active-low:

```text
PC13 LOW  -> LED ON
PC13 HIGH -> LED OFF
```

One complete ON/OFF cycle takes about one second.

## 3. Hardware

No external components are required.

```text
Blue Pill onboard LED -> PC13
```

Board configuration:

```c
#define BOARD_STATUS_LED_PORT         MCAL_GPIO_PORT_C
#define BOARD_STATUS_LED_PIN          (13U)
#define BOARD_STATUS_LED_ACTIVE_LEVEL MCAL_GPIO_LEVEL_LOW
```

## 4. Compile-Time Configuration

```c
#define BOARD_HSE_FREQUENCY_HZ (8000000UL)
#define BOARD_TARGET_CLOCK_HZ   (72000000UL)
#define BOARD_TIMEBASE_HZ       (1000UL)

#define APPLICATION_BLINK_PERIOD_MS (500UL)
```

The example requires a 1 kHz board timebase so one tick equals one millisecond.

## 5. Startup Flow

```text
Reset_Handler
    |
main()
    |
disable global IRQ
    |
system_init()
    |
    +--> board_init()
    |      +--> try HSE + PLL -> 72 MHz
    |      +--> fallback to HSI 8 MHz on failure
    |      +--> board_led_init()
    |      +--> board_timebase_init(actual SYSCLK)
    |
    +--> time_service_init()
    +--> indication_service_init()
    +--> application_init()
    |
enable global IRQ
    |
super-loop
```

The global IRQ lifecycle is explicit: SysTick cannot increment until
`system_init()` completes and interrupts are enabled.

## 6. Clock Setup

The Board layer requests:

```text
HSE = 8 MHz
target SYSCLK = 72 MHz
```

MCAL RCC configures HSE/PLL. If the external crystal path fails, the board calls
`mcal_rcc_use_hsi()`.

The active source can be queried through `board_get_clock_source()`.

All timebase configuration uses the actual clock reported by MCAL, not a
hard-coded 72 MHz assumption.

## 7. GPIO LED

The BSP calls `mcal_gpio_configure()` for PC13.

The active level is defined by the board:

```text
logical ON -> PC13 LOW
logical OFF -> PC13 HIGH
```

Application never sees the pin number or active polarity.

## 8. SysTick Timebase

`board_timebase_init()` calls:

```c
mcal_systick_init(core_clock_hz, BOARD_TIMEBASE_HZ);
```

The MCAL programs the Cortex-M3 SysTick registers:

```text
CTRL
LOAD
VAL
```

and enables:

```text
CLKSOURCE
TICKINT
ENABLE
```

`SysTick_Handler()` increments a volatile tick counter.

With a 1 kHz timebase, the Service interprets ticks directly as milliseconds.

## 9. Application State

The Application stores the last toggle timestamp.

Every call to `application_process()` asks whether 500 ms has elapsed.

```text
period due?
    |
    +--> no  -> return
    |
    +--> yes -> toggle logical indication
```

No busy delay is used.

## 10. Architecture

```text
Application
    |
    +--> Time Service ------> Board Timebase -----> MCAL SysTick
    |
    +--> Indication Service -> Board LED ---------> MCAL GPIO
                                                       |
                                                       v
                                                Platform registers
```

## 11. Interrupt

Only SysTick is active.

Ownership:

```text
SysTick core peripheral
    |
MCAL SysTick
    |
SysTick_Handler()
```

The ISR only increments the tick counter.

## 12. Idle and Panic

The example uses:

```c
cortex_m3_nop();
```

for both normal idle and the panic loop.

Panic disables global interrupts first.

This is intentionally debug-friendly for an ST-Link connection without NRST.

## 13. Recommended Reading Order

1. `app/src/application.c`
2. `services/src/time_service.c`
3. `services/src/indication_service.c`
4. `bsp/bluepill/src/board_led.c`
5. `bsp/bluepill/src/board_timebase.c`
6. `mcal/src/mcal_gpio.c`
7. `mcal/src/mcal_systick.c`
8. `mcal/src/mcal_rcc.c`
9. `platform/device/stm32f103xb/include/...`
10. `system/system_init.c`

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

## 14. Troubleshooting

### LED Does Not Blink

Check:

1. `system_init()` succeeds;
2. global IRQ is enabled after initialization;
3. `SysTick_Handler()` is reached;
4. the SysTick counter increases;
5. `application_process()` runs continuously;
6. the logical indication toggles;
7. PC13 changes level.

### LED Is Always On or Always Off

Check active-low handling and PC13 configuration.

Do not "fix" the Application by inverting its logical state. Polarity belongs
in BSP.

### Debugger Is Difficult to Attach

Check SWD wiring and confirm OpenOCD uses:

```tcl
reset_config none
```

The `NOP` idle/panic policy is intended to keep attach behavior predictable.

## 15. Extension Exercises

- change the blink period;
- add asymmetric ON/OFF timing;
- move the LED to another pin without changing Application;
- replace SysTick with a timer-backed board timebase;
- expose the active clock source in a debug variable.

## 16. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
