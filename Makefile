##########################################################################################################################
# STM32C031C6 - Lab 2
##########################################################################################################################

TARGET = LR1_

DEBUG = 1
OPT = -Og

BUILD_DIR = build

######################################
# Sources
######################################

C_SOURCES = \
Core/Src/main.c \
App/Src/app.c \
Devices/Src/mpu6050.c \
Core/Src/stm32c0xx_it.c \
Core/Src/stm32c0xx_hal_msp.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_adc.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_adc_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_dma.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_dma_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_rcc.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_rcc_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_flash.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_flash_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_gpio.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_pwr.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_pwr_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_cortex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_exti.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_i2c.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_i2c_ex.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_uart.c \
Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_uart_ex.c \
Core/Src/system_stm32c0xx.c \
Core/Src/sysmem.c \
Core/Src/syscalls.c

ASM_SOURCES = \
startup_stm32c031xx.s

######################################
# Toolchain
######################################

PREFIX = arm-none-eabi-

ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

######################################
# Host test toolchain
######################################

HOST_CC = gcc

TEST_BUILD_DIR = $(BUILD_DIR)/tests
TEST_TARGET = $(TEST_BUILD_DIR)/test_mpu6050

HOST_CFLAGS = \
-Wall \
-Wextra \
-std=c11

######################################
# CPU
######################################

CPU = -mcpu=cortex-m0plus
MCU = $(CPU) -mthumb

######################################
# Defines
######################################

C_DEFS = \
-DUSE_HAL_DRIVER \
-DSTM32C031xx

######################################
# Includes
######################################

C_INCLUDES = \
-ICore/Inc \
-IApp/Inc \
-IDevices/Inc \
-IDrivers/STM32C0xx_HAL_Driver/Inc \
-IDrivers/STM32C0xx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32C0xx/Include \
-IDrivers/CMSIS/Include

######################################
# Flags
######################################

CFLAGS = $(MCU) \
$(C_DEFS) \
$(C_INCLUDES) \
$(OPT) \
-Wall \
-fdata-sections \
-ffunction-sections

ASFLAGS = $(MCU) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
ASFLAGS += -g -gdwarf-2
endif

CFLAGS += -MMD -MP

######################################
# Linker
######################################

LDSCRIPT = STM32C031xx_FLASH.ld

LIBS = -lc -lm -lnosys

LDFLAGS = $(MCU) \
-specs=nano.specs \
-T$(LDSCRIPT) \
$(LIBS) \
-Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
-Wl,--gc-sections

######################################
# Objects
######################################

C_OBJECTS = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

######################################
# Firmware build
######################################

all: \
$(BUILD_DIR)/$(TARGET).elf \
$(BUILD_DIR)/$(TARGET).hex \
$(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(BIN) $< $@

######################################
# Host tests
######################################

test: $(TEST_TARGET)
	$(TEST_TARGET)

$(TEST_TARGET): tests/Src/test_mpu6050.c
	@mkdir -p $(TEST_BUILD_DIR)
	$(HOST_CC) $(HOST_CFLAGS) $< -o $@

######################################
# Clean
######################################

clean:
	rm -rf $(BUILD_DIR)

######################################
# Dependencies
######################################

-include $(C_OBJECTS:.o=.d)

.PHONY: all test clean