# ========== Ayarlar ==========
TARGET = kernel
ISO = burstkernel.iso
BUILD = build
SRC_ASM = boot/boot.S
SRC_C = kernel/kernel.c
OBJ_ASM = $(BUILD)/boot.o
OBJ_C = $(BUILD)/kernel.o
LINKER = linker.ld

# Derleyiciler
CC = gcc
LD = ld
AS = gcc
CFLAGS = -m32 -ffreestanding -O0 -Wall -Wextra -nostdlib
LDFLAGS = -m elf_i386

# ========== Kurallar ==========
.PHONY: all clean run

all: $(ISO)

# ISO üretimi
$(ISO): $(TARGET).bin
	mkdir -p iso/boot/grub
	cp $< iso/boot/kernel
	cp boot/grub/grub.cfg iso/boot/grub
	grub-mkrescue -o $@ iso

# ELF kernel.bin üretimi
$(TARGET).bin: $(OBJ_ASM) $(OBJ_C)
	$(LD) $(LDFLAGS) -T $(LINKER) -o $@ $^

# boot.S derle
$(OBJ_ASM): $(SRC_ASM)
	mkdir -p $(BUILD)
	$(AS) $(CFLAGS) -c $< -o $@

# kernel.c derle
$(OBJ_C): $(SRC_C)
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Temizlik
clean:
	rm -rf $(BUILD) $(TARGET).bin $(ISO) iso

# QEMU ile çalıştır
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)
