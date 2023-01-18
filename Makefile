# Check arguments
ifeq ($(HW),LAUNCHPAD) # HW argument
TARGET_HW=launchpad
else ifeq ($(HW),NSUMO)
TARGET_HW=nsumo
else ifeq ($(MAKECMDGOALS),clean)
else ifeq ($(MAKECMDGOALS),cppcheck)
else ifeq ($(MAKECMDGOALS),format)
# HW argument not required for this rule
else
$(error "Must pass HW=LAUNCHPAD or HW=NSUMO")
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
OBJ_DIR = $(BUILD_DIR)/obj
TI_CCS_DIR = $(TOOLS_DIR)/ccs1210/ccs
DEBUG_BIN_DIR = $(TI_CCS_DIR)/ccs_base/DebugServer/bin
DEBUG_DRIVERS_DIR = $(TI_CCS_DIR)/ccs_base/DebugServer/drivers

LIB_DIRS = $(MSPGCC_INCLUDE_DIR)
INCLUDE_DIRS = $(MSPGCC_INCLUDE_DIR) \
			   ./src \
			   ./external/ \
			   ./external/printf

# Toolchain
CC = $(MSPGCC_BIN_DIR)/msp430-elf-gcc
RM = rm
DEBUG = LD_LIBRARY_PATH=$(DEBUG_DRIVERS_DIR) $(DEBUG_BIN_DIR)/mspdebug
CPPCHECK = cppcheck
FORMAT = clang-format-12

# Files
TARGET = $(BUILD_DIR)/bin/$(TARGET_HW)/$(TARGET_NAME)

SOURCES_WITH_HEADERS = \
		src/common/assert_handler.c \
		src/drivers/mcu_init.c \
		src/drivers/io.c \
		src/drivers/led.c \
		src/app/drive.c \
		src/app/enemy.c \

ifndef TEST
SOURCES = \
		src/main.c \
		$(SOURCES_WITH_HEADERS)
else
SOURCES = \
		src/test/test.c \
		$(SOURCES_WITH_HEADERS)
# Delete object file to force rebuild when changing test (there is probably a better way...)
$(shell rm -f $(BUILD_DIR)/obj/src/test/test.o)
endif

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
	$(TEST_DEFINE)

# Static Analysis
## Don't check the msp430 helper headers (they have a LOT of ifdefs)
CPPCHECK_INCLUDES = ./src
CPPCHECK_IGNORE = external/printf
CPPCHECK_FLAGS = \
	--quiet --enable=all --error-exitcode=1 \
	--inline-suppr \
	--suppress=missingIncludeSystem \
	--suppress=unmatchedSuppression \
	--suppress=unusedFunction \
	$(addprefix -I,$(CPPCHECK_INCLUDES)) \
	$(addprefix -i,$(CPPCHECK_IGNORE))

# Flags
MCU = msp430g2553
WFLAGS = -Wall -Wextra -Werror -Wshadow
CFLAGS = -mmcu=$(MCU) $(WFLAGS) -fshort-enums $(addprefix -I,$(INCLUDE_DIRS)) $(DEFINES) -Og -g
LDFLAGS = -mmcu=$(MCU) $(DEFINES) $(addprefix -L,$(LIB_DIRS))

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
.PHONY: all clean flash cppcheck format

all: $(TARGET)

clean:
	@$(RM) -rf $(BUILD_DIR)

flash: $(TARGET)
	@$(DEBUG) tilib "prog $(TARGET)"

cppcheck:
	@$(CPPCHECK) $(CPPCHECK_FLAGS) $(SOURCES)

format:
	@$(FORMAT) -i $(SOURCES) $(HEADERS)
