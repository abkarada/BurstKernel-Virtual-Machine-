#────────────────────────  Network OS Makefile  ────────────────────────
#  make         → derle + ISO üret
#  make run     → TAP arayüzü ile QEMU önyükle (rate-limitsiz)
#  make slirp   → SLiRP-NAT ile (ekstra parametre gerektirmeyen) QEMU
#  make clean   → obj / iso / isodir sil
#────────────────────────────────────────────────────────────────────────

.PHONY: all run slirp clean

# ── Dosya adları
ELF     := kernel.elf
ISO     := kernel.iso
LWIP_DIR := lwip/src
LWIP_SRCS := $(LWIP_DIR)/core/init.c $(LWIP_DIR)/core/def.c \
             $(LWIP_DIR)/core/mem.c $(LWIP_DIR)/core/memp.c \
             $(LWIP_DIR)/core/netif.c $(LWIP_DIR)/core/pbuf.c \
             $(LWIP_DIR)/core/stats.c $(LWIP_DIR)/core/sys.c \
             $(LWIP_DIR)/core/tcp.c $(LWIP_DIR)/core/tcp_in.c \
             $(LWIP_DIR)/core/tcp_out.c $(LWIP_DIR)/core/udp.c \
             $(LWIP_DIR)/core/ip.c $(LWIP_DIR)/core/inet_chksum.c \
             $(LWIP_DIR)/core/ipv4/autoip.c $(LWIP_DIR)/core/ipv4/dhcp.c \
             $(LWIP_DIR)/core/ipv4/etharp.c $(LWIP_DIR)/core/ipv4/icmp.c \
             $(LWIP_DIR)/core/ipv4/igmp.c $(LWIP_DIR)/core/ipv4/ip4_frag.c \
             $(LWIP_DIR)/core/ipv4/ip4.c $(LWIP_DIR)/core/ipv4/ip4_addr.c \
             $(LWIP_DIR)/core/timeouts.c \
             $(LWIP_DIR)/netif/ethernet.c \
             port/ethernetif.c

LWIP_OBJS := $(LWIP_SRCS:.c=.o)
OBJS    := boot.o kernel.o idt.o keyboard.o isr.o mm.o e1000.o libc.o cli.o telnetd.o ata.o nat.o $(LWIP_OBJS)
LINKER  := linker.ld
GRUBCFG := grub.cfg
ISODIR  := isodir/boot
GRUBDIR := $(ISODIR)/grub

# ── Derleme Bayrakları
CC      := gcc
AS      := as
LD      := ld

# 64-bit kernel derleme bayrakları
CFLAGS  := -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
           -ffreestanding -nostdlib -std=gnu99 -fno-stack-protector \
           -Wall -Wextra -c -I. -I./port -I./$(LWIP_DIR)/include

# ── Varsayılan hedef
all: $(ELF)

# ── boot.s → boot.o  (32-to-64 bit bootstrap)
boot.o: boot.s
	@echo "[AS]  $@"
	$(AS) $< -o $@

isr.o: isr.s
	@echo "[AS]  $@"
	$(AS) $< -o $@

# ── C kaynak dosyaları
%ata.o: ata.c
	@echo "[CC]  $@"
	@$(CC) $(CFLAGS) $< -o $@

nat.o: nat.c
	@echo "[CC]  $@"
	@$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	@echo "[CC]  $@"
	$(CC) $(CFLAGS) $< -o $@

# ── Link → kernel.elf
$(ELF): $(LINKER) $(OBJS)
	@echo "[LD]  $@"
	$(LD) -m elf_x86_64 -T $< $(filter %.o,$^) -o $@

# ── ISO üret
$(ISO): $(ELF) $(GRUBCFG)
	@echo "[ISO] $@"
	@mkdir -p $(GRUBDIR)
	cp    $(ELF)     $(ISODIR)/
	cp    $(GRUBCFG) $(GRUBDIR)/
	grub-mkrescue -o $@ isodir 2>/dev/null

# ── Objcopy (64-bit ELF'i 32-bit kılıfına koyar, QEMU boot etsin diye)
kernel32.elf: $(ELF)
	@echo "[OBJCOPY] $@"
	objcopy -O elf32-i386 $< $@

# ── QEMU / TAP (rate-limitsiz) --> make run
run: kernel32.elf
	# TAP arayüzü yoksa otomatik oluştur
	sudo ip link show tap0 >/dev/null 2>&1 || \
	    (sudo ip tuntap add tap0 mode tap && sudo ip link set tap0 up)
	qemu-system-x86_64 \
	  -m 128M \
	  -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
	  -device e1000,netdev=net0 \
	  -kernel kernel32.elf \
	  -serial stdio -no-reboot -display none

# ── QEMU / SLiRP (hız sınırlı ama ayarsız) --> make slirp
config.img:
	@echo "[DD]  Creating config.img (1MB)"
	@dd if=/dev/zero of=config.img bs=512 count=2048 status=none

slirp: kernel32.elf config.img
	qemu-system-x86_64 \
	  -m 128M \
	  -nic user,model=e1000,hostfwd=tcp::2323-:23 \
	  -nic user,model=e1000 \
	  -drive file=config.img,format=raw,if=ide \
	  -kernel kernel32.elf \
	  -serial stdio -no-reboot

# ── Temizlik
clean:
	rm -rf $(OBJS) $(ELF) $(ISO) isodir
