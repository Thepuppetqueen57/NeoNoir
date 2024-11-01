TARGET = NoirOS-X16
BOOTASM = src/boot.asm
BIN_DIR = bin
ISO_DIR = $(BIN_DIR)/iso
TARGET_BIN = $(BIN_DIR)/$(TARGET)
TARGET_ISO = $(ISO_DIR)/$(TARGET).iso

all: $(TARGET_ISO)

$(TARGET_BIN): $(BIN_DIR)/boot.bin
	cat $^ > $@

$(BIN_DIR)/boot.bin: $(BOOTASM)
	mkdir -p $(BIN_DIR)
	nasm -f bin $< -o $@

$(TARGET_ISO): $(TARGET_BIN)
	mkdir -p $(ISO_DIR)
	dd if=$(TARGET_BIN) of=$(TARGET_ISO) conv=notrunc

run:
	qemu-system-x86_64 bin/iso/NoirOS-X16.iso

clean:
	rm -rf $(BIN_DIR)