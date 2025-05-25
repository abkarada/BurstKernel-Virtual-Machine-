# ========== Ayarlar ==========
TARGET = kernel
ISO = burstkernel.iso
BUILD = build
SRC_ASM = boot/boot.S
SRC_C = $(filter-out kernel/task.c, $(wildcard kernel/*.c))
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
	@rm -rf $(BUILD) $(TARGET).bin $(ISO) iso gen_taskpool taskpool.bin

all: $(BUILD) $(OBJ_ASM) $(OBJ_C) $(TASKPOOL_OBJ)
	$(LD) -m elf_i386 -T $(LINKER) -o $(TARGET).bin $(OBJ_ASM) $(OBJ_C) $(TASKPOOL_OBJ)
	@mkdir -p iso/boot/grub
	@cp $(TARGET).bin iso/boot/kernel
	@cp boot/grub/grub.cfg iso/boot/grub
	@grub-mkrescue -o $(ISO) iso

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/boot.o: $(SRC_ASM)
	$(AS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TASKPOOL_OBJ): $(TASKPOOL_BIN)
	$(LD) -m elf_i386 -r -b binary $(TASKPOOL_BIN) -o $(TASKPOOL_OBJ)

$(TASKPOOL_BIN): gen_taskpool
	@./gen_taskpool

gen_taskpool: gen_taskpool.c
	$(CC) -o $@ $<
