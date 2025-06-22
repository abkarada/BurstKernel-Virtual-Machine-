#────────────────────────  BurstKernel Makefile  ────────────────────────
#  make         → derle + ISO üret
#  make run     → TAP arayüzü ile QEMU önyükle (rate-limitsiz)
#  make slirp   → SLiRP-NAT ile (ekstra parametre gerektirmeyen) QEMU
#  make clean   → obj / iso / isodir sil
#────────────────────────────────────────────────────────────────────────

.PHONY: all run slirp clean

# ── Dosya adları
ELF     := kernel.elf
ISO     := kernel.iso
OBJS    := boot.o kernel.o
LINKER  := linker.ld
GRUBCFG := grub.cfg
ISODIR  := isodir/boot
GRUBDIR := $(ISODIR)/grub

# ── Varsayılan hedef
all: $(ISO)

# ── boot.s → boot.o  (multiboot header içermeli)
boot.o: boot.s
	@echo "[AS]  $@"
	as --32 $< -o $@

# ── kernel.c → kernel.o
kernel.o: kernel.c
	@echo "[CC]  $@"
	gcc -m32 -ffreestanding -nostdlib -std=gnu99 -fno-stack-protector \
	    -c $< -o $@

# ── Link → kernel.elf
$(ELF): $(LINKER) $(OBJS)
	@echo "[LD]  $@"
	ld -m elf_i386 -T $< $(filter %.o,$^) -o $@

# ── ISO üret
$(ISO): $(ELF) $(GRUBCFG)
	@echo "[ISO] $@"
	@mkdir -p $(GRUBDIR)
	cp    $(ELF)     $(ISODIR)/
	cp    $(GRUBCFG) $(GRUBDIR)/
	grub-mkrescue -o $@ isodir 2>/dev/null

# ── QEMU / TAP (rate-limitsiz) --> make run
run: $(ISO)
	# TAP arayüzü yoksa otomatik oluştur
	sudo ip link show tap0 >/dev/null 2>&1 || \
	    (sudo ip tuntap add tap0 mode tap && sudo ip link set tap0 up)
	qemu-system-i386 \
	  -m 128M \
	  -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
	  -device e1000,netdev=net0 \
	  -cdrom $(ISO) \
	  -serial stdio -no-reboot

# ── QEMU / SLiRP (hız sınırlı ama ayarsız) --> make slirp
slirp: $(ISO)
	qemu-system-i386 \
	  -m 128M \
	  -nic user,model=e1000 \
	  -cdrom $(ISO) \
	  -serial stdio -no-reboot

# ── Temizlik
clean:
	rm -rf $(OBJS) $(ELF) $(ISO) isodir
