# ----------------------------- Build targets -----------------------------
#   make            -> derle + ISO üret
#   make run        -> QEMU’da önyükle
#   make clean      -> tüm çıktıları sil
# -------------------------------------------------------------------------

.PHONY : all run clean

# ---------- Temel dosya isimleri ----------
ISO         := kernel.iso
ELF         := kernel.elf
OBJS        := boot.o kernel.o
LINKER      := linker.ld
GRUBCFG     := grub.cfg
ISODIR      := isodir/boot
GRUBDIR     := $(ISODIR)/grub

# ---------- Varsayılan hedef ----------
all : $(ISO)

# ---------- Assemble boot.s ----------
boot.o : boot.s
	@echo "[AS]  $@"
	as --32 $< -o $@

# ---------- Compile kernel.c ----------
kernel.o : kernel.c
	@echo "[CC]  $@"
	gcc -m32 -ffreestanding -nostdlib -fno-stack-protector \
	    -std=gnu99 -c $< -o $@

# ---------- Link ELF ----------
$(ELF) : $(LINKER) $(OBJS)
	@echo "[LD]  $@"
	ld -m elf_i386 -T $(LINKER) $(OBJS) -o $@

# ---------- Build GRUB ISO ----------
$(ISO) : $(ELF) $(GRUBCFG)
	@echo "[ISO] $@"
	@mkdir -p $(GRUBDIR)
	cp $(ELF)      $(ISODIR)/
	cp $(GRUBCFG)  $(GRUBDIR)/
	grub-mkrescue -o $@ isodir 2>/dev/null

# ---------- QEMU çalıştır ----------
run : $(ISO)
	qemu-system-i386 \
	  -m 128M \
	  -netdev user,id=n0 \
	  -device e1000,netdev=n0 \
	  -cdrom  $(ISO) \
	  -serial stdio -no-reboot

# ---------- Temizlik ----------
clean :
	rm -rf $(OBJS) $(ELF) $(ISO) isodir

