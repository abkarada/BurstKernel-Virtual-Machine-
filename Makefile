#───────────────────────────────────────────────────────────
#  BurstKernel  –  kök Makefile (i686 ELF cross-toolchain)
#───────────────────────────────────────────────────────────

# Derleyiciler
CC      ?= i686-elf-gcc
AS      ?= i686-elf-as
LD      ?= i686-elf-ld

# Ortak bayraklar
CFLAGS  = -ffreestanding -m32 -O2 -Wall -Wextra -Iinclude
ASFLAGS = --32
LDFLAGS = -T linker.ld -nostdlib -m elf_i386

# VM kimliği derleme sırasında “make VM_ID=17” şeklinde geçilir
VM_ID   ?= 0
CFLAGS += -DVM_ID=$(VM_ID)

# Derlenecek nesneler
OBJS = \
    boot/boot.o \
    kernel/kmain.o \
    kernel/pci.o \
    kernel/e1000.o \
    kernel/task.o \
    kernel/serial.o \
    kernel/string.o

# Eğer “net.c” hâlâ kullanılıyorsa ekle; yoksa liste dışı bırak
# OBJS += kernel/net.o

# Varsayılan hedef
all: kernel.elf

#────────── Nesne kuralları ──────────
boot/boot.o: boot/boot.S
	$(AS) $(ASFLAGS) -o $@ $<

# Tüm *.c dosyaları için
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

#────────── Bağlama ──────────────────
kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# (İstersen ELF’ten ham binary çıkar)
kernel.bin: kernel.elf
	i686-elf-objcopy -O binary $< $@

# Temizlik
clean:
	rm -f $(OBJS) kernel.elf kernel.bin
