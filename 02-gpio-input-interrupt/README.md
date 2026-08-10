# 02-gpio-input-interrupt — GPIO Input + EXTI + Debounce Outside the ISR

## 1. Learning Objectives

This example adds an external push button using EXTI0.

You will learn:

- PA0 input with pull-up;
- AFIO EXTI routing;
- falling-edge interrupt configuration;
- NVIC priority;
- ISR event capture;
- thread-mode debounce;
- Event Service usage;
- logical LED control.

## 2. Wiring

```text
PA0 ---- push button ---- GND
```

PA0 is biased HIGH internally.

Therefore:

```text
released -> HIGH
pressed  -> LOW
```

The falling edge represents a press.

## 3. Behavior

One valid debounced press toggles the PC13 LED once.

Holding the button does not repeat.

Mechanical bounce may generate several physical edges, but only one logical
press should reach Application.

## 4. Config

```c
#define BOARD_USER_BUTTON_PORT         MCAL_GPIO_PORT_A
#define BOARD_USER_BUTTON_PIN          (0U)
#define BOARD_USER_BUTTON_ACTIVE_LEVEL MCAL_GPIO_LEVEL_LOW
#define BOARD_USER_BUTTON_EXTI_LINE    (0U)
#define BOARD_USER_BUTTON_IRQ_PRIORITY (2U)

#define SERVICE_EVENT_QUEUE_CAPACITY    (16U)
#define BUTTON_SERVICE_DEBOUNCE_TIME_MS (30UL)
```

The board timebase remains 1 kHz.

## 5. Initialization Flow

```text
board_init()
    |
    +--> clock setup/fallback
    +--> board LED
    +--> board timebase
    +--> board button
            |
            +--> GPIO PA0 pull-up
            +--> AFIO mapping
            +--> EXTI0 falling edge
            +--> NVIC priority/enable
```

Services initialize after board resources.

Global IRQ remains disabled until `system_init()` completes.

## 6. Register-Level GPIO Input Pull-Up

STM32F1 input pull-up mode uses:

```text
MODE = input
CNF  = input with pull-up/pull-down
ODR bit = 1 -> pull-up
```

MCAL GPIO hides this encoding behind a logical mode and pull level.

The BSP only declares "active-low button on PA0."

## 7. AFIO + EXTI Setup

EXTI line 0 must be mapped to GPIOA through AFIO.

The MCAL configures:

```text
AFIO EXTICR
EXTI IMR
EXTI FTSR
EXTI PR
```

for:

```text
PA0 -> EXTI0 -> falling edge -> interrupt enabled
```

## 8. ISR Ownership

The lowest EXTI-owning module handles `EXTI0_IRQHandler()`.

ISR work:

```text
check pending
clear pending
record press edge
return
```

No debounce occurs in the handler.

## 9. Event Handoff from ISR to Thread Mode

The low-level edge is converted into a thread-mode event.

The Button Service consumes raw edges and eventually publishes a logical button
event through the Event Service.

This keeps the Application independent from EXTI.

## 10. Debounce Algorithm

When a raw press edge arrives:

```text
start/restart 30 ms debounce window
```

After 30 ms:

```text
read PA0
    |
still pressed?
    |
    +--> yes -> publish button-pressed event
    +--> no  -> ignore
```

No busy delay is used.

## 11. Application

Application consumes the logical pressed event.

```text
BUTTON_PRESSED event
    |
toggle INDICATION_STATUS
```

Application does not know:

- PA0;
- EXTI0;
- active-low polarity;
- debounce duration.

## 12. Architecture

```text
Application
    |
    +--> Event Service
    +--> Button Service ------> Board Button ------> MCAL GPIO/EXTI
    +--> Indication Service --> Board LED ---------> MCAL GPIO
    +--> Time Service --------> Board Timebase ----> MCAL SysTick
```

## 13. Event Service

The Event Service contains a bounded static queue.

Configured capacity:

```text
16 events
```

The Service separates event publication from Application consumption and can
be extended for additional input sources.

## 14. Idle Behavior

`system_idle()` executes `cortex_m3_nop()`.

The super-loop remains responsive while EXTI and SysTick operate through
interrupts.

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

## 15. Step-by-Step Test

1. Verify PA0 is HIGH when released.
2. Press the switch.
3. Confirm `EXTI0_IRQHandler()` is reached.
4. Hold the switch.
5. Confirm one logical LED toggle.
6. Release.
7. Press again.
8. Confirm one additional toggle.
9. Observe raw bounce with a logic analyzer if desired.

## 16. Troubleshooting

### Press Has No Effect

Check PA0 wiring, pull-up mode, AFIO mapping, EXTI pending state, NVIC enable,
and the Service processing path.

### LED Toggles Multiple Times per Press

Check the 30 ms debounce configuration and confirm Application consumes only the
debounced Event Service output.

### EXTI ISR Hits but LED Does Not Change

Then the low-level interrupt path is working.

Inspect Button Service debounce state, Event Service queue, and
Indication Service calls.

## 17. Extension Exercises

- add release events;
- add long-press detection;
- add double-click detection;
- add a second button;
- move to another EXTI line;
- add event-overflow diagnostics.

## 18. Related Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)
