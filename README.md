# STM32F103 Register-Level Bare-Metal Project Template

A minimal, buildable project skeleton for the **STM32F103C8T6 (Blue Pill)**, written in C11 and ARM assembly.

This branch is intended to be cloned as the starting point for new register-level embedded projects. It provides the startup code, linker script, vector table, build system, architecture boundaries, and empty application hooks, while deliberately leaving peripheral drivers and product behavior to the user.

## Design Goals

- Direct register-level development without STM32 HAL, LL, SPL, libopencm3, Arduino Core, or an RTOS
- Clear separation between portable application logic and hardware-specific code
- Closed layered architecture with compile-time dependency checks
- No dynamic memory allocation
- A small, understandable startup and runtime environment
- A reusable base for GPIO, UART, SPI, I2C, CAN, ADC, DMA, timer, and other projects

## Target

| Item | Value |
|---|---|
| MCU | STM32F103C8T6 |
| Board | STM32F103C8T6 Blue Pill |
| CPU | Arm Cortex-M3 |
| Flash | 64 KiB |
| SRAM | 20 KiB |
| Language | C11 and GNU assembler |
| Build system | GNU Make |
| Debug interface | SWD through ST-Link or a compatible probe |

## What This Template Contains

The default firmware performs only the minimum runtime sequence:

```text
Reset_Handler
    |
    v
Initialize .data and .bss
    |
    v
main()
    |
    v
board_init()          // empty board hook
    |
    v
application_init()    // empty application hook
    |
    v
Super-loop
    |
    +--> application_process()  // empty
    |
    +--> WFI
```

The template does **not** include a functional GPIO, SysTick, UART, communication protocol, external-device driver, or example application.

The MCU clock tree is also intentionally left unconfigured. Add an RCC driver in the MCAL layer when the project requires a specific system clock.

## Layered Architecture

Runtime dependencies must point downward:

```text
Application
    |
    v
Services
    |
    v
BSP / ECU Abstraction
    |
    v
MCAL
    |
    v
Device / Architecture
    |
    v
Hardware
```

The `system/` directory is the composition root. It is the only runtime area allowed to initialize and connect modules from multiple layers.

Startup files, linker scripts, configuration headers, and development tools are build-time infrastructure rather than runtime application layers.

### Dependency Rules

| Layer | May depend on | Must not depend on |
|---|---|---|
| Application | Services, portable common types | BSP, ECUAL, MCAL, STM32 or Cortex-M headers |
| Services | BSP, ECUAL, portable common utilities | Application, raw registers |
| BSP / ECUAL | MCAL, portable common utilities | Services, Application |
| MCAL | Device layer, architecture layer, low-level common utilities | BSP, Services, Application |
| Device / Architecture | Standard integer types and compiler primitives | Any upper layer |
| Common | Standard language headers | Hardware-specific modules |

Run the dependency checker with:

```bash
make check-layers
```

## Repository Layout

```text
.
├── app/                         Application behavior and state machines
├── services/                    High-level services and middleware
├── ecual/                       Drivers for external devices
├── bsp/bluepill/                Blue Pill board abstraction
├── mcal/                        STM32 peripheral drivers
├── platform/
│   ├── arch/cortex-m3/          Cortex-M3 core definitions
│   └── device/stm32f103xb/      STM32F103 register and IRQ definitions
├── common/                      Portable utilities and common types
├── config/                      Compile-time project configuration
├── system/                      Composition root and main super-loop
├── startup/                     Reset handler and vector table
├── linker/                      STM32F103C8T6 linker script
├── tests/                       Host-side tests
├── tools/                       OpenOCD, GDB, and architecture-check scripts
├── Makefile
└── README.md
```

Empty directories are retained in Git with `.gitkeep` files.

## Prerequisites

Install the following tools:

- GNU Arm Embedded Toolchain:
  - `arm-none-eabi-gcc`
  - `arm-none-eabi-objcopy`
  - `arm-none-eabi-objdump`
  - `arm-none-eabi-size`
- GNU Make
- Python 3
- OpenOCD for flashing and debugging
- `arm-none-eabi-gdb` or `gdb-multiarch` for debugging
- ST-Link or another OpenOCD-compatible SWD probe

Verify the main tools:

```bash
arm-none-eabi-gcc --version
make --version
python3 --version
openocd --version
```

## Clone the Template Branch

```bash
git clone \
    --branch template \
    --single-branch \
    https://github.com/haikevins/stm32f1-register-level-baremetal.git \
    my-stm32-project

cd my-stm32-project
```

To start a completely independent repository:

```bash
rm -rf .git
git init
git add .
git commit -m "chore: initialize STM32F103 bare-metal project"
```

## Build

```bash
make
```

Generated files:

```text
build/firmware.elf
build/firmware.bin
build/firmware.hex
build/firmware.map
build/firmware.lst
```

Use a custom firmware name without editing the Makefile:

```bash
make PROJECT=my_firmware
```

Clean generated files:

```bash
make clean
```

Display section sizes:

```bash
make size
```

Display the project tree:

```bash
make tree
```

## Flash

Connect the Blue Pill to an ST-Link through SWD, then run:

```bash
make flash
```

Erase the device:

```bash
make erase
```

The default OpenOCD configuration is located at:

```text
tools/openocd/bluepill_stlink.cfg
```

## Debug

Start OpenOCD in the first terminal:

```bash
make debug-server
```

Start GDB in a second terminal:

```bash
make debug
```

The Makefile prefers `arm-none-eabi-gdb` and falls back to `gdb-multiarch` when the dedicated ARM debugger is unavailable.

## Starting a New Project

A typical implementation sequence is:

1. Define product behavior in `app/`.
2. Add high-level APIs in `services/`.
3. Add board resources in `bsp/bluepill/`.
4. Add external-device drivers in `ecual/`.
5. Add register-level peripheral drivers in `mcal/`.
6. Extend the STM32F103 register map in `platform/device/stm32f103xb/` only when necessary.
7. Connect initialization in `system/system_init.c`.
8. Keep interrupt handlers inside the lowest layer that owns the peripheral.
9. Keep all super-loop processing non-blocking.

### Example Module Placement

| Feature | Recommended location |
|---|---|
| Product state machine | `app/` |
| Time, event, protocol, diagnostics | `services/` |
| Board LED or push button | `bsp/bluepill/` |
| SSD1306, MPU6050, external EEPROM | `ecual/` |
| GPIO, USART, SPI, I2C, CAN, ADC, DMA | `mcal/` |
| STM32 register structures and bit masks | `platform/device/stm32f103xb/` |
| Cortex-M3 NVIC, SysTick, SCB helpers | `platform/arch/cortex-m3/` |
| Ring buffer, CRC, fixed-size queue | `common/` |

## Interrupt Policy

An interrupt handler may:

- Read and acknowledge peripheral flags
- Move data into a statically allocated low-level buffer
- Update a low-level counter or status flag

An interrupt handler must not:

- Include application headers
- Run application state machines
- Parse high-level protocols
- Allocate dynamic memory
- Block
- Call upward into Services or Application

Upper layers should obtain data through polling APIs or explicitly designed queues during normal thread-mode execution.

## Documentation

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/adding_a_module.md`](docs/adding_a_module.md)
- [`docs/porting_guide.md`](docs/porting_guide.md)

## Examples

Functional projects based on this template are available on the `examples` branch:

```bash
git clone \
    --branch examples \
    --single-branch \
    https://github.com/haikevins/stm32f1-register-level-baremetal.git \
    stm32f1-examples
```

## License

This project is licensed under the MIT License. See [`LICENSE`](LICENSE) for details.
