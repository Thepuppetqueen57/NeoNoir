# Compiler and linker settings
CC = gcc
AS = nasm
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -c
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Object files
OBJ = build/boot.o build/kernel.o

# Default target
all: build/noiros.iso

# Create build directory
build:
	mkdir -p build
	mkdir -p iso/boot/grub

# Compile kernel
build/kernel.o: src/kernel.c
	$(CC) $(CFLAGS) src/kernel.c -o build/kernel.o

# Compile boot code
build/boot.o: src/boot.asm
	$(AS) $(ASFLAGS) src/boot.asm -o build/boot.o

# Link everything together
build/noiros.bin: $(OBJ)
	ld $(LDFLAGS) $(OBJ) -o build/noiros.bin

# Create ISO
build/noiros.iso: build build/noiros.bin
	cp build/noiros.bin iso/boot/
	grub-mkrescue -o build/noiros.iso iso

# Clean build files
clean:
	rm -rf build/*
	rm -f iso/boot/noiros.bin

run: build/noiros.iso
	qemu-system-i386 -cdrom build/noiros.iso

.PHONY: all clean run