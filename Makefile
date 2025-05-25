# ========== Ayarlar ==========
TARGET = kernel
ISO = burstkernel.iso
BUILD = build
SRC_ASM = boot/boot.S
SRC_C = $(filter-out kernel/task.c, $(wildcard kernel/*.c))  # task.c hariç tutuldu
OBJ_ASM = $(BUILD)/boot.o
OBJ_C = $(patsubst kernel/%.c,$(BUILD)/%.o,$(SRC_C))
TASKPOOL_BIN = taskpool.bin
TASKPOOL_OBJ = $(BUILD)/taskpool.o
LINKER = linker.ld

# Derleyiciler
CC = gcc
LD = ld
AS = gcc
CFLAGS = -m32 -ffreestanding -O0 -Wall -Wextra -nostdlib
LDFLAGS = -m elf_i386

# ========== Kurallar ==========
.PHONY: all clean run

clean:
	rm -rf $(BUILD) $(TARGET).bin $(ISO) iso gen_taskpool taskpool.bin

all: $(ISO)

# ISO üretimi
$(ISO): $(TARGET).bin
	mkdir -p iso/boot/grub
	cp $< iso/boot/kernel
	cp boot/grub/grub.cfg iso/boot/grub
	grub-mkrescue -o $@ iso

# kernel.bin üretimi
$(TARGET).bin: $(OBJ_ASM) $(OBJ_C) $(TASKPOOL_OBJ)
	$(LD) $(LDFLAGS) -T $(LINKER) -o $@ $^

# ASM derle
$(OBJ_ASM): $(SRC_ASM)
	mkdir -p $(BUILD)
	$(AS) $(CFLAGS) -c $< -o $@

# C kaynaklarını derle
$(BUILD)/%.o: kernel/%.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# taskpool.bin üret ve objeye dönüştür (i386 uyumlu)
$(TASKPOOL_BIN):
	@gcc gen_taskpool.c -o gen_taskpool
	./gen_taskpool

$(TASKPOOL_OBJ): $(TASKPOOL_BIN)
	ld -m elf_i386 -r -b binary $< -o $@

# Temizlik
clean:
	rm -rf $(BUILD) $(TARGET).bin $(ISO) iso gen_taskpool taskpool.bin

# QEMU ile çalıştır
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)
