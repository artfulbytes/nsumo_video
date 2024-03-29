# Check arguments
GOALS_WITHOUT_HW = clean cppcheck format terminal tests
GOAL_HAS_TARGET = $(filter $(MAKECMDGOALS),$(GOALS_WITHOUT_HW))
ifeq ($(GOAL_HAS_TARGET),)

ifeq ($(HW),LAUNCHPAD) # HW argument
TARGET_HW=launchpad
else ifeq ($(HW),NSUMO)
TARGET_HW=nsumo
else
$(error "Must pass HW=LAUNCHPAD or HW=NSUMO")
endif

endif
TARGET_NAME=$(TARGET_HW)

ifneq ($(TEST),) # TEST argument
ifeq ($(findstring test_,$(TEST)),)
$(error "TEST=$(TEST) is invalid (test function must start with test_)")
else
TARGET_NAME=$(TEST)
endif
endif

# Directories
TOOLS_DIR = ${TOOLS_PATH}
MSPGCC_ROOT_DIR = $(TOOLS_DIR)/msp430-gcc
MSPGCC_BIN_DIR = $(MSPGCC_ROOT_DIR)/bin
MSPGCC_INCLUDE_DIR = $(MSPGCC_ROOT_DIR)/include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/$(TARGET_HW)/obj
TI_CCS_DIR = $(TOOLS_DIR)/ccs1210/ccs
DEBUG_BIN_DIR = $(TI_CCS_DIR)/ccs_base/DebugServer/bin
DEBUG_DRIVERS_DIR = $(TI_CCS_DIR)/ccs_base/DebugServer/drivers

LIB_DIRS = $(MSPGCC_INCLUDE_DIR)
INCLUDE_DIRS = $(MSPGCC_INCLUDE_DIR) \
			   ./src \
			   ./external/ \
			   ./

# Toolchain
CC = $(MSPGCC_BIN_DIR)/msp430-elf-gcc
RM = rm
DEBUG = LD_LIBRARY_PATH=$(DEBUG_DRIVERS_DIR) $(DEBUG_BIN_DIR)/mspdebug
CPPCHECK = cppcheck
FORMAT = clang-format-12
SIZE = $(MSPGCC_BIN_DIR)/msp430-elf-size
READELF = $(MSPGCC_BIN_DIR)/msp430-elf-readelf
ADDR2LINE = $(MSPGCC_BIN_DIR)/msp430-elf-addr2line

# Files
TARGET = $(BUILD_DIR)/$(TARGET_HW)/bin/$(TARGET_NAME)

SOURCES_WITH_HEADERS = \
		src/common/assert_handler.c \
		src/common/ring_buffer.c \
		src/common/trace.c \
		src/common/sleep.c \
		src/common/enum_to_string.c \
		src/drivers/mcu_init.c \
		src/drivers/io.c \
		src/drivers/led.c \
		src/drivers/uart.c \
		src/drivers/ir_remote.c \
		src/drivers/pwm.c \
		src/drivers/tb6612fng.c \
		src/drivers/adc.c \
		src/drivers/qre1113.c \
		src/drivers/i2c.c \
		src/drivers/vl53l0x.c \
		src/drivers/millis.c \
		src/app/drive.c \
		src/app/enemy.c \
		src/app/line.c \
		src/app/timer.c \
		src/app/input_history.c \
		src/app/state_machine.c \
		src/app/state_wait.c \
		src/app/state_search.c \
		src/app/state_attack.c \
		src/app/state_retreat.c \
		src/app/state_manual.c \
		external/printf/printf.c \

ifndef TEST
MAIN_FILE = src/main.c
else
MAIN_FILE = src/test/test.c
# Touch test.c to force rebuild every time in case TEST define changed
$(shell touch src/test/test.c)
endif

SOURCES = \
		$(MAIN_FILE) \
		$(SOURCES_WITH_HEADERS)

HEADERS = \
		$(SOURCES_WITH_HEADERS:.c=.h) \
		src/common/defines.h \

OBJECT_NAMES = $(SOURCES:.c=.o)
OBJECTS = $(patsubst %,$(OBJ_DIR)/%,$(OBJECT_NAMES))

# Defines
HW_DEFINE = $(addprefix -D,$(HW))
TEST_DEFINE = $(addprefix -DTEST=,$(TEST))
DEFINES = \
	$(HW_DEFINE) \
	$(TEST_DEFINE) \
	-DPRINTF_INCLUDE_CONFIG_H \
	-DDISABLE_ENUM_STRINGS \
	-DDISABLE_TRACE \

# Static Analysis
## Don't check the msp430 helper headers (they have a LOT of ifdefs)
CPPCHECK_INCLUDES = ./src ./
IGNORE_FILES_FORMAT_CPPCHECK = \
	external/printf/printf.h \
	external/printf/printf.c
SOURCES_FORMAT_CPPCHECK = $(filter-out $(IGNORE_FILES_FORMAT_CPPCHECK),$(SOURCES))
HEADERS_FORMAT = $(filter-out $(IGNORE_FILES_FORMAT_CPPCHECK),$(HEADERS))
CPPCHECK_FLAGS = \
	--quiet --enable=all --error-exitcode=1 \
	--inline-suppr \
	--suppress=missingIncludeSystem \
	--suppress=unmatchedSuppression \
	--suppress=unusedFunction \
	$(addprefix -I,$(CPPCHECK_INCLUDES)) \

# Flags
MCU = msp430g2553
WFLAGS = -Wall -Wextra -Werror -Wshadow
CFLAGS = -mmcu=$(MCU) $(WFLAGS) -fshort-enums $(addprefix -I,$(INCLUDE_DIRS)) $(DEFINES) -Og -g
LDFLAGS = -mmcu=$(MCU) $(DEFINES) $(addprefix -L,$(LIB_DIRS)) $(addprefix -I,$(INCLUDE_DIRS))

# Build
## Linking
$(TARGET): $(OBJECTS) $(HEADERS)
	echo $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ -o $@

## Compiling
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $^

# Phonies
.PHONY: all clean flash cppcheck format size symbols addr2line terminal tests

all: $(TARGET)

clean:
	@$(RM) -rf $(BUILD_DIR)

flash: $(TARGET)
	@$(DEBUG) tilib "prog $(TARGET)"

cppcheck:
	@$(CPPCHECK) $(CPPCHECK_FLAGS) $(SOURCES_FORMAT_CPPCHECK)

format:
	@$(FORMAT) -i $(SOURCES_FORMAT_CPPCHECK) $(HEADERS_FORMAT)

size: $(TARGET)
	@$(SIZE) $(TARGET)

symbols: $(TARGET)
	# List symbols table sorted by size
	@$(READELF) -s $(TARGET) | sort -n -k3

addr2line: $(TARGET)
	@$(ADDR2LINE) -e $(TARGET) $(ADDR)

terminal:
	@# Running without sudo requires udev rule under /etc/udev/rules.d
	@echo "picocom -b 115200 /dev/ttyUSB0"
	@sleep 1
	@picocom -b 115200 /dev/ttyUSB0

tests:
	@# Build all tests
	@tools/build_tests.sh
