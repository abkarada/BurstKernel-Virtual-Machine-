#───────────────────────────────────────────────────────────
#  BurstKernel   –   kök Makefile
#───────────────────────────────────────────────────────────
CC      ?= i686-elf-gcc
AS      ?= i686-elf-as
LD      ?= i686-elf-ld

CFLAGS  = -ffreestanding -m32 -O2 -Wall -Wextra -Iinclude
ASFLAGS = --32
LDFLAGS = -T linker.ld -nostdlib -m elf_i386

OBJS = \
    boot/boot.o \
    kernel/kmain.o \
    kernel/pci.o \
    kernel/e1000.o \
    kernel/net.o \
    kernel/serial.o \
    kernel/string.o

# Eğer eski kernel.c’yi kullanmayacaksanız listede tutmayın.
# OBJS += kernel/kernel.o   ← ekler veya kaldırırsınız

all: kernel.elf

boot/boot.o: boot/boot.S
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) kernel.elf

