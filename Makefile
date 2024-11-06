# Compiler and linker settings
CC = gcc
AS = nasm
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -c
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Directories
SRC_DIR = src
BUILD_DIR = build

# Find all source files
C_SRCS := $(wildcard $(SRC_DIR)/*.c)
ASM_SRCS := $(wildcard $(SRC_DIR)/*.asm)
HEADERS := $(wildcard $(SRC_DIR)/*.h)

# Generate object file names
C_OBJS := $(C_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
ASM_OBJS := $(ASM_SRCS:$(SRC_DIR)/%.asm=$(BUILD_DIR)/%.o)

# All object files
OBJ = $(C_OBJS) $(ASM_OBJS)

# Default target
all: build/noiros.iso

# Create build directory
build:
	mkdir -p build
	mkdir -p iso/boot/grub

# Compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

# Compile Assembly files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	$(AS) $(ASFLAGS) $< -o $@

# Link everything together
build/noiros.bin: $(OBJ)
	ld $(LDFLAGS) $(OBJ) -o build/noiros.bin

# Create ISO
build/noiros.iso: build build/noiros.bin
	cp build/noiros.bin iso/boot/
	grub-mkrescue -o build/noiros.iso iso

# Clean build files
clean:
	rm -rf build
	rm -f iso/boot/noiros.bin

run: build/noiros.iso
	qemu-system-i386 -cdrom build/noiros.iso -machine pc -enable-kvm -audio alsa

# Print variables for debugging
print-%:
	@echo $* = $($*)

.PHONY: all clean run print-%