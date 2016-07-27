
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
INC = -I$(SRC_DIR) -I$(SRC_DIR)/cpu/$(CPU_SERIES)

# general flags
CFLAGS += -Wall -fPIC
CFLAGS += -fno-builtin -ffunction-sections -fdata-sections
# cpu related flags
CFLAGS += $(INC) -mthumb -mcpu=$(CPU_CORE)
# lpcopen required definitions
CFLAGS += -DCORE_M0

# linker flags
MAP_FILE = $(OUT_DIR)/$(PROJECT).map
LINKER_FILE = $(SRC_DIR)/cpu/$(CPU_SERIES)/$(CPU).ld
LDFLAGS += -nostdlib -T $(LINKER_FILE) -Xlinker -Map=$(MAP_FILE) -Xlinker --gc-sections

# libraries
LIBS =

# source and object files
SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/cpu/$(CPU_SERIES)/*.c)
ASM = $(wildcard $(SRC_DIR)/cpu/$(CPU_SERIES)/*.s)
OBJ = $(SRC:.c=.o) $(ASM:.s=.o)
ELF = $(OUT_DIR)/$(PROJECT).elf
BIN = $(OUT_DIR)/$(PROJECT).bin

# rules
all: prebuild $(ELF)

prebuild:
	@mkdir -p $(OUT_DIR)

$(ELF): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(LIBS) -o $@
	$(OBJCOPY) -O binary $@ $(BIN)
	$(CHECKSUM) $(BIN)
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.s
	$(CC) -c -x assembler-with-cpp $(CFLAGS) -o "$@" "$<"

install: all
	$(ISP) $(BIN)

clean:
	rm -rf $(OBJ) $(OUT_DIR)
