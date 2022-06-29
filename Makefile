GITHUB_DEPS += simplerobot/build-scripts
GITHUB_DEPS += simplerobot/hw-test-agent
GITHUB_DEPS += simplerobot/rlm3-hardware
GITHUB_DEPS += simplerobot/logger
GITHUB_DEPS += simplerobot/test-stm32
include ../build-scripts/build/release/include.make

TOOLCHAIN_PATH = /opt/gcc-arm-none-eabi-7-2018-q2-update/bin/arm-none-eabi-

BUILD_DIR = build
LIBRARY_BUILD_DIR = $(BUILD_DIR)/library
TEST_BUILD_DIR = $(BUILD_DIR)/test
RELEASE_DIR = $(BUILD_DIR)/release

CC = $(TOOLCHAIN_PATH)gcc
AS = $(TOOLCHAIN_PATH)gcc -x assembler-with-cpp
SZ = $(TOOLCHAIN_PATH)size
HX = $(TOOLCHAIN_PATH)objcopy -O ihex
BN = $(TOOLCHAIN_PATH)objcopy -O binary -S

MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard 
OPTIONS = -fdata-sections -ffunction-sections -Wall -Werror -DUSE_FULL_ASSERT=1

LIBRARIES = \
	-lc \
	-lm \
	-lstdc++

DEFINES = \
	-DUSE_HAL_DRIVER \
	-DSTM32F427xx \
	-DTEST \
	-DUSE_FULL_ASSERT=1
	
INCLUDES = \
	-I$(LIBRARY_BUILD_DIR) \
	-I$(PKG_RLM3_HARDWARE_DIR) \
	-I$(PKG_LOGGER_DIR) \
	-I$(PKG_TEST_STM32_DIR)
	
SOURCE_DIR = source
MAIN_SOURCE_DIR = $(SOURCE_DIR)/main
TEST_SOURCE_DIR = $(SOURCE_DIR)/test

LIBRARY_FILES = $(notdir $(wildcard $(MAIN_SOURCE_DIR)/*))

TEST_SOURCE_DIRS = $(MAIN_SOURCE_DIR) $(TEST_SOURCE_DIR) $(PKG_RLM3_HARDWARE_DIR) $(PKG_LOGGER_DIR) $(PKG_TEST_STM32_DIR)
TEST_SOURCE_FILES = $(notdir $(wildcard $(TEST_SOURCE_DIRS:%=%/*.c) $(TEST_SOURCE_DIRS:%=%/*.cpp) $(TEST_SOURCE_DIRS:%=%/*.s)))
TEST_O_FILES = $(addsuffix .o,$(basename $(TEST_SOURCE_FILES)))
TEST_LD_FILE = $(wildcard $(PKG_RLM3_HARDWARE_DIR)/*.ld)

VPATH = $(LIBRARY_BUILD_DIR) : $(TEST_SOURCE_DIR) : $(PKG_RLM3_HARDWARE_DIR) : $(PKG_LOGGER_DIR) : $(PKG_TEST_STM32_DIR)

.PHONY: default all library test release clean

default : all

all : release

library : $(LIBRARY_FILES:%=$(LIBRARY_BUILD_DIR)/%)

$(LIBRARY_BUILD_DIR)/% : $(MAIN_SOURCE_DIR)/% | $(LIBRARY_BUILD_DIR)
	cp $< $@

$(LIBRARY_BUILD_DIR) :
	mkdir -p $@

test : library $(TEST_BUILD_DIR)/test.bin $(TEST_BUILD_DIR)/test.hex
	$(PKG_HW_TEST_AGENT_DIR)/sr-hw-test-agent --run --test-timeout=15 --system-frequency=180m --trace-frequency=2m --board RLM36 --file $(TEST_BUILD_DIR)/test.bin	

$(TEST_BUILD_DIR)/test.bin : $(TEST_BUILD_DIR)/test.elf
	$(BN) $< $@

$(TEST_BUILD_DIR)/test.hex : $(TEST_BUILD_DIR)/test.elf
	$(HX) $< $@

$(TEST_BUILD_DIR)/test.elf : $(TEST_O_FILES:%=$(TEST_BUILD_DIR)/%)
	$(CC) $(MCU) $(TEST_LD_FILE:%=-T%) -Wl,--gc-sections $^ $(LIBRARIES) -s -o $@ -Wl,-Map=$@.map,--cref
	$(SZ) $@

$(TEST_BUILD_DIR)/%.o : %.c Makefile | $(TEST_BUILD_DIR)
	$(CC) -c $(MCU) $(OPTIONS) $(DEFINES) $(INCLUDES) -MMD -g -Og -gdwarf-2 $< -o $@

$(TEST_BUILD_DIR)/%.o : %.cpp Makefile | $(TEST_BUILD_DIR)
	$(CC) -c $(MCU) $(OPTIONS) $(DEFINES) $(INCLUDES) -std=c++11 -MMD -g -Og -gdwarf-2 $< -o $@

$(TEST_BUILD_DIR)/%.o : %.s Makefile | $(TEST_BUILD_DIR)
	$(AS) -c $(MCU) $(OPTIONS) $(DEFINES) -MMD $< -o $@

$(TEST_BUILD_DIR) :
	mkdir -p $@

release : test $(LIBRARY_FILES:%=$(RELEASE_DIR)/%)

$(RELEASE_DIR)/% : $(LIBRARY_BUILD_DIR)/% | $(RELEASE_DIR)
	cp $< $@
	
$(RELEASE_DIR) :
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

-include $(wildcard $(TEST_BUILD_DIR)/*.d)


