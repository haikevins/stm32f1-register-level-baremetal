# Architecture — 08-adc-dma

## 1. Hardware Data Path

```text
TIM3 update @ 1 kHz
    |
ADC1 channel 0
    |
ADC1->DR
    |
DMA1 Channel 1
    |
64-sample circular buffer
```

Individual samples move without CPU intervention.

## 2. Software Dependency Graph

```text
Application
    |
    +--> ADC Service ------> Board ADC/DMA
    |                           |
    |                           +--> MCAL ADC
    |                           +--> MCAL DMA
    |                           +--> MCAL Timer
    |                           +--> MCAL NVIC
    |
    +--> Indication Service -> Board LED -> MCAL GPIO
```

## 3. Ownership Table

| Concern | Owner |
|---|---|
| sample timing | TIM3/MCAL |
| ADC conversion | ADC MCAL |
| DMA transfer | DMA MCAL |
| IRQ/block publication | Board ADC/DMA |
| statistics | ADC Service |
| threshold policy | Application |
| PC13 polarity | BSP |

## 4. ISR Placement in the Current Code

The strong DMA1 Channel 1 handler is in the Board ADC/DMA ownership module
because that module owns the complete ADC+DMA board resource.

It calls the MCAL only to take/clear DMA event flags.

No Service/Application call occurs in the ISR.

## 5. Concurrency Zones

Three storage zones:

```text
DMA circular buffer
BSP stable completed block
Service processing buffer
```

The block-ready flag bridges ISR and thread mode.

## 6. Overrun Semantics

If a new half completes before the previous published block is consumed:

```text
overrun_count++
newest block replaces pending block
```

The design favors current data over an unbounded backlog.

## 7. Sampling Determinism

TIM3 determines sample timing.

Therefore thread-mode scheduling jitter does not directly change conversion
times.

Long interrupt masking can still delay DMA servicing and cause overrun.

## 8. Service Decoupling

ADC Service receives raw blocks and produces:

```text
average
minimum
maximum
millivolts
sequence
```

Application does not know DMA buffer geometry or ADC registers.

## 9. Error Propagation

DMA transfer errors become a counter exposed through the Service.

Initialization/calibration failures return `false` and lead to
`system_panic()`.

## 10. Memory Use

Major sample storage:

```text
DMA buffer:      64 * 2 = 128 bytes
published block: 32 * 2 = 64 bytes
Service buffer:  32 * 2 = 64 bytes
```

The RAM cost buys a simple ownership model.

## 11. Extension Options

### Multi-Channel Scan

Configure multiple ADC sequence ranks and define the interleaved DMA layout.

### No-Copy Ping-Pong

Process DMA halves directly with strict ownership and timing guarantees.

This reduces copies but increases concurrency complexity.

### Queue Multiple Blocks

Store several completed blocks if every block must be preserved.

This increases SRAM use and requires a clear queue overflow policy.

## 12. Boundary Rule

Preserve:

```text
hardware movement -> BSP/MCAL
sample interpretation -> Service
product policy -> Application
```

Do not move ADC/DMA register logic upward for convenience.
