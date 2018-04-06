include ../../py/mkenv.mk

CROSS = 0

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

#FROZEN_DIR = frozen_scripts
#FROZEN_MPY_DIR = frozen_mpy_modules


# include py core make definitions
include $(TOP)/py/py.mk

ifeq ($(CROSS), 1)
CROSS_COMPILE = /cygdrive/c/EmBitz_share/em_armgcc/bin/arm-none-eabi-
endif

EXTMOD_DIR = $(TOP)/extmod
POKITTO_DIR = ../../../PokittoLib2/Pokitto/POKITTO_LIBS/MicroPython
MBED_HAL_DIR = ../../../PokittoLib2/Pokitto/mbed-pokitto/hal
MBED_HAL_DIR2 = ../../../PokittoLib2/Pokitto/mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X
MBED_API_DIR = ../../../PokittoLib2/Pokitto/mbed-pokitto/api
MBED_CMSIS_DIR = ../../../PokittoLib2/Pokitto/mbed-pokitto/targets/cmsis
MBED_CMSIS_DIR2 = ../../../PokittoLib2/Pokitto/mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(EXTMOD_DIR)
INC += -I$(POKITTO_DIR)
INC += -I$(MBED_HAL_DIR)
INC += -I$(MBED_HAL_DIR2)
INC += -I$(MBED_API_DIR)
INC += -I$(MBED_CMSIS_DIR)
INC += -I$(MBED_CMSIS_DIR2)

ifeq ($(CROSS), 1)
DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CFLAGS_CORTEX_M4 = -mthumb -mcpu=cortex-m0plus -fsingle-precision-constant -fno-exceptions -fomit-frame-pointer -Wdouble-promotion 
CFLAGS = $(INC) -Wall -Werror -std=gnu11 -nostdlib $(CFLAGS_CORTEX_M4) $(COPT)
LDFLAGS = -nostdlib -T stm32f405.ld -Map=$@.map --cref --gc-sections
else
LD = gcc
CFLAGS = -m32 $(INC) -Wall -Werror -std=gnu11 $(COPT)
LDFLAGS = -m32 -Wl,-Map=$@.map,--cref -Wl,--gc-sections
endif

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
#CFLAGS += -O0 -ggdb
CFLAGS += -Og -ggdb3
else
#CFLAGS += -Os -DNDEBUG
CFLAGS += -O3 -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

LIBS =

SRC_C = \
	main.c \
	uart_core.c \
	modules/modmachine.c \
	modules/machine_pin.c \
	modules/modupygame.c \
	modules/modupygame_mixer.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c
	#$(BUILD)/frozen_mpy.c

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

SRC_QSTR += modules/modmachine.c modules/machine_pin.c modules/modupygame.c modules/modupygame_mixer.c

#ifeq ($(CROSS), 1)
#all: $(BUILD)/firmware.dfu
#else
#all: $(BUILD)/firmware.elf
#endif

#$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
#	$(ECHO) "MISC freezing bytecode"
#	$(Q)$(PYTHON) ../tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@
	

#$(BUILD)/firmware.elf: $(OBJ)
#	$(ECHO) "LINK $@"
#	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
#	$(Q)$(SIZE) $@

#$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
#	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $(BUILD)/firmware.bin

#$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
#	$(ECHO) "Create $@"
#	$(Q)$(PYTHON) $(DFU) -b 0x08000000:$(BUILD)/firmware.bin $@

#deploy: $(BUILD)/firmware.dfu
#	$(ECHO) "Writing $< to the board"
#	$(Q)$(PYTHON) $(PYDFU) -u $<

# Run emulation build on a POSIX system with suitable terminal settings
#run:
#	stty raw opost -echo
#	build/firmware.elf
#	@echo Resetting terminal...
# This sleep is useful to spot segfaults
#	sleep 1
#	reset

#test: $(BUILD)/firmware.elf
#	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! #\\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"

LIBMICROPYTHON_EXTRA_CMD = bash -x copy_lib_and_qstr.sh

include $(TOP)/py/mkrules.mk
