# Example 09 architecture

```text
Application  <-- uses can_service, time_service, indication_service
    |                         |                    |
Services (CAN, time, LED) --- BSP (CAN/clock/GPIO/SysTick/LED)
                                 |
                               MCAL (bxCAN/SysTick/RCC/GPIO/NVIC)
                                 |
                          platform device + Cortex-M3
```

## Ownership

- `mcal_can.c`: CAN register programming (init/bit timing/filter/tx mailbox0/FIFO0/ESR).
- `board_can.c`: PB8/PB9 binding and RX0 ISR, single-frame mailbox with latest-wins policy.
- `can_service.c`: minimal payload policy, counters and semantic validation.
- `application.c`: TX periodic scheduler or RX-only role via `APPLICATION_CAN_ROLE`; exports volatile GDB variables.
- `time_service.c` and board SysTick are copied from Example 01; clock, GPIO, linker, startup and debugging scaffolding from Example 08.

## Concurrency

```text
CAN HW FIFO0 --IRQ20--> board RX ISR --(latest-frame mailbox)--> thread service
CAN TX mailbox0 <------ thread MCAL TX submit / completion poll
SysTick IRQ ----------------> 1 ms volatile counter -----> periodic scheduler
```

- `board_can_take_received()` copies and clears the mailbox with PRIMASK save/restore.
- ISR only copies a valid frame and tracks overwritten frames.
- No service or application state is modified from RX interrupt context.
- TX completion is polled in thread mode, with no unbounded wait. `NART=1` disables automatic retransmission; a missing ACK produces a completion error.
- Startup enables global IRQs only after successful `system_init()`.

## Invariants

1. RX mailbox is written only from RX0 ISR; consumed only in thread mode.
2. CAN frames are Standard (11-bit) Data frames, with DLC up to 8.
3. CAN bit timing is derived from the active APB1 clock; 125 kbit/s is exact for 36 or 8 MHz APB1.
4. At most one pending software TX request exists at a time.
5. No heap, HAL, LL or SPL.

## Caveats

FIFO0 can overrun under load; the ISR drains at most 3 slots per invocation. This educational example intentionally reports latest-wins overwrites instead of guaranteeing every frame. A robust product might use an SPSC queue, timestamping, CAN error interrupts and backpressure.
