# Architecture

## Layer map

```text
Application
    |
    v
Services
    |
    v
BSP / ECUAL
    |
    v
MCAL
    |
    v
Device / Cortex-M3 architecture
    |
    v
Hardware
```

## Button path

```text
application_process()
    |
    v
button_service_take_press()
    |
    +--> board_button_take_press_event()
    |        |
    |        v
    |    mcal_exti_take_event()
    |
    +--> time_service_elapsed_ms()
    |
    +--> board_button_is_pressed()
             |
             v
         mcal_gpio_read()
```

## Interrupt rule

`EXTI0_IRQHandler()` belongs to MCAL. It may clear EXTI flags and store a
low-level event. It must not call the BSP, Services, or Application.

Debounce and product behavior are intentionally deferred to thread mode.

## Dependency rules

| Module | May depend on |
|---|---|
| Application | Services, Common, Config |
| Services | BSP, ECUAL, Common, Config |
| BSP / ECUAL | MCAL, Common, Config |
| MCAL | Device, Architecture, Common, Config |
| Device / Architecture | Standard integer headers |
| Common | Portable standard headers |

`system/` is the composition root and owns initialization order.
