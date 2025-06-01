#!/usr/bin/env bash
# start_machines.sh  –  BurstKernel otomatik derleme + QEMU başlatıcı

set -euo pipefail

### ——— Ayarlanabilir değerler ———————————————
RAM_FILE=/dev/shm/shrmem          # Paylaşımlı tmpfs dosyası
RAM_MB=128                        # QEMU -m parametresi (MB)
RAM_SIZE=$((RAM_MB*1024*1024))    # Bayt karşılığı
SERIAL_BASE=7000                  # Telnet port başlangıcı
MAKE_CMD="make -s"                # Sessiz make
QEMU="qemu-system-i386"           # i686 çekirdek
QEMU_CPU="-cpu host"              # KVM varsa -enable-kvm eklenir
### ————————————————————————————————————————————————

# ——— VM sayısı argümanı ———
[[ $# -eq 1 && $1 =~ ^[0-9]+$ ]] || { echo "Kullanım: $0 <VM_SAYISI>"; exit 1; }
VM_TOTAL=$1
echo "[+] $VM_TOTAL VM hazırlanıyor…"

# ——— Paylaşımlı RAM dosyası oluştur ———
echo "[+] Paylaşımlı RAM dosyası: $RAM_FILE (${RAM_MB} MiB)"
truncate -s "$RAM_SIZE" "$RAM_FILE"

# ——— Görev bloklarını yaz ———
echo "[+] writer_ram ile görev bloğu yerleştiriliyor…"
gcc writer_ram.c -o writer_ram
./writer_ram "$VM_TOTAL"

# ——— Ctrl-C temizleme ———
PIDS=()
cleanup() {
    echo "[*] VM’ler kapatılıyor…"
    for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null || true; done
    rm -f writer_ram
    exit
}
trap cleanup INT TERM

# ——— Her VM’i derle + başlat ———
for ((id=0; id<VM_TOTAL; id++)); do
    echo "[+] VM_ID=$id çekirdeği derleniyor…"
    $MAKE_CMD clean >/dev/null
    $MAKE_CMD VM_ID=$id  >/dev/null

    ELF="kernel_vm${id}.elf"
    mv kernel.elf "$ELF"

    TELNET_PORT=$((SERIAL_BASE + id))
    echo "    • QEMU port $TELNET_PORT → telnet 127.0.0.1 $TELNET_PORT"

    $QEMU \
      -enable-kvm $QEMU_CPU -smp 1 -m "${RAM_MB}M" \
      -object memory-backend-file,id=ram0,share=on,mem-path="$RAM_FILE",size="$RAM_SIZE" \
      -machine pc,accel=kvm,memory-backend=ram0 \
      -kernel "$ELF" \
      -nographic \
      -serial telnet:127.0.0.1:"$TELNET_PORT",server,nowait \
      -device e1000 \
      &
    PIDS+=($!)
done

echo "[✓] Tüm VM’ler başlatıldı.  Çıkmak için Ctrl-C."
wait            # QEMU süreçleri bitene dek bekle
