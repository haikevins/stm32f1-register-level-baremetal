PROJECT := firmware
NODE ?= tx
ifeq ($(NODE),tx)
ROLE := 1U
else ifeq ($(NODE),rx)
ROLE := 2U
else
$(error NODE must be tx or rx)
endif
BUILD_DIR := build/$(NODE)
PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
AS := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE := $(PREFIX)size
ARM_GDB := $(shell command -v $(PREFIX)gdb 2>/dev/null)
ifeq ($(ARM_GDB),)
GDB ?= gdb-multiarch
else
GDB ?= $(PREFIX)gdb
endif
OPENOCD ?= openocd
CPU_FLAGS := -mcpu=cortex-m3 -mthumb
CPPFLAGS := -Iapp/include -Iservices/include -Iecual/include \
    -Ibsp/bluepill/include -Imcal/include \
    -Iplatform/arch/cortex-m3/include \
    -Iplatform/device/stm32f103xb/include \
    -Icommon/include -Iconfig -Isystem \
    -DAPPLICATION_CAN_ROLE=$(ROLE)
CFLAGS := $(CPU_FLAGS) -std=c11 -Og -g3 -ffreestanding -fno-builtin \
    -Wa,--noexecstack \
    -ffunction-sections -fdata-sections -fno-common \
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wundef \
    -Werror=implicit-function-declaration
# Emit non-executable .note.GNU-stack metadata (do not hide linker warnings).
ASFLAGS := $(CPU_FLAGS) -x assembler-with-cpp -g3 -Wa,--noexecstack
LDFLAGS := $(CPU_FLAGS) -nostartfiles -nostdlib \
    -Tlinker/stm32f103c8t6.ld -Wl,--gc-sections -Wl,--build-id=none \
    -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map
LDLIBS := -lgcc
C_SOURCES := $(wildcard app/src/*.c) $(wildcard services/src/*.c) \
    $(wildcard ecual/src/*.c) $(wildcard bsp/bluepill/src/*.c) \
    $(wildcard mcal/src/*.c) \
    $(wildcard platform/arch/cortex-m3/src/*.c) \
    $(wildcard platform/device/stm32f103xb/src/*.c) \
    $(wildcard common/src/*.c) $(wildcard system/*.c) $(wildcard startup/*.c)
ASM_SOURCES := $(wildcard startup/*.S)
C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)
DEPS := $(OBJECTS:.o=.d)
ELF := $(BUILD_DIR)/$(PROJECT).elf
BIN := $(BUILD_DIR)/$(PROJECT).bin
HEX := $(BUILD_DIR)/$(PROJECT).hex
LST := $(BUILD_DIR)/$(PROJECT).lst
.PHONY: all clean flash erase debug-server debug size check-toolchain check-layers tree
all: check-layers $(ELF) $(BIN) $(HEX) $(LST)
check-layers:
	python3 tools/scripts/check_layers.py
check-toolchain:
	@command -v $(CC) >/dev/null 2>&1 || { echo 'Missing $(CC)'; exit 1; }
$(ELF): check-toolchain $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(CPPFLAGS) $(ASFLAGS) -MMD -MP -c $< -o $@
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@
$(LST): $(ELF)
	$(OBJDUMP) -d -S $< > $@
size: $(ELF)
	$(SIZE) -A $(ELF)
flash: $(ELF)
	$(OPENOCD) -f tools/openocd/bluepill_stlink.cfg -c "program $(ELF) verify reset exit"
erase:
	$(OPENOCD) -f tools/openocd/bluepill_stlink.cfg -c "init; reset halt; stm32f1x mass_erase 0; reset run; exit"
debug-server:
	$(OPENOCD) -f tools/openocd/bluepill_stlink.cfg
debug: $(ELF)
	$(GDB) -x tools/gdb/debug.gdb $(ELF)
tree:
	@find . -path './build' -prune -o -print | sort
clean:
	rm -rf build
-include $(DEPS)
