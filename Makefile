CC := arm-none-eabi-gcc

# tools definitions
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
CHECKSUM = tools/checksum
ISP = tools/nxp-usb-isp

# project definitions
PROJECT = footswitch
SRC_DIR = src
OUT_DIR = out

# cpu definitions
CPU = LPC11U24
CPU_SERIES = LPC11Uxx
CPU_CORE=cortex-m0

# flags for debugging
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -g -DDEBUG
else
CFLAGS += -O3
endif

# include directories
INC = -I$(SRC_DIR) -I$(SRC_DIR)/cpu/$(CPU_SERIES) -I$(SRC_DIR)/cc

# cpu related flags
CPU_FLAGS = -mthumb -mcpu=$(CPU_CORE)

# general flags
CFLAGS += $(INC) -Wall -Wextra -Wpointer-arith -Wredundant-decls -std=gnu99
CFLAGS += -fno-builtin -ffunction-sections -fdata-sections
# lpcopen required definitions
CFLAGS += -DCORE_M0
ifeq ($(CCC_ANALYZER_OUTPUT_FORMAT),)
CFLAGS += $(CPU_FLAGS)
else
CFLAGS += -DCCC_ANALYZER -Wshadow -Wno-attributes
endif

# linker flags
MAP_FILE = $(OUT_DIR)/$(PROJECT).map
LINKER_FILE = $(SRC_DIR)/cpu/$(CPU_SERIES)/$(CPU).ld
LDFLAGS += -nostdlib -T $(LINKER_FILE) -Xlinker -Map=$(MAP_FILE) -Xlinker --gc-sections
LDFLAGS += $(CPU_FLAGS) -specs=nano.specs
LDFLAGS += -Wl,--start-group -lgcc -lc -lm -lrdimon -Wl,--end-group

# libraries
LIBS =

# source and object files
SRC = $(wildcard $(SRC_DIR)/*.c)
SRC += $(wildcard $(SRC_DIR)/cpu/$(CPU_SERIES)/*.c) $(wildcard $(SRC_DIR)/cc/*.c)
ASM = $(wildcard $(SRC_DIR)/cpu/$(CPU_SERIES)/*.s)
OBJ = $(SRC:.c=.o) $(ASM:.s=.o)
ELF = $(OUT_DIR)/$(PROJECT).elf

# rules
all: prebuild $(ELF)

prebuild:
	@echo $(SRC)
	@mkdir -p $(OUT_DIR)

$(ELF): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(LIBS) -o $@
	$(OBJCOPY) -O binary $@ $(@:.elf=.bin)
	$(CHECKSUM) $(@:.elf=.bin)
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# ignore warnings for 3rd-party CPU code
src/cpu/%.o: src/cpu/%.c
	$(CC) $(CFLAGS) -o $@ -c $< -Wno-unused-parameter

%.o: %.s
	$(CC) -c -x assembler-with-cpp $(CFLAGS) -o "$@" "$<"

install: all
	$(ISP) $(OUT_DIR)/$(PROJECT).bin

clean:
	rm -rf $(OBJ) $(OUT_DIR)
