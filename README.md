# NKernel

NKernel, NAT Traversal (Ağ Adresi Çevirisi Geçişi) problemlerini çözmek amacıyla ortaya çıkmış eğlenceli bir "side-project" (yan proje) unikernel denemesidir. 

Başlangıçta "BurstKernel" adıyla, yüksek hızlı paket yönlendirmesi yapabilen bağımsız bir çekirdek (kernel) modülü olarak tasarlandı. Amacı; standart işletim sistemlerindeki sistem çağrıları (syscall), kesmeler (interrupt) ve bağlam değiştirme (context-switch) gibi zaman maliyetlerini aradan çıkararak, karşılıklı simetrik NAT senaryolarında NATGhost algoritmasını uygulayabilmekti.

Projenin teknik arka planını ve neden böyle bir mimariye (doğrudan bare-metal'de çalışmaya) ihtiyaç duyulduğunu detaylarıyla asıl raporda bulabilirsiniz:
- [NATGhost Algoritma Raporu (Orijinal DOCX)](NATGhost_Algoritma_Raporu.docx)
- [NATGhost Algorithm Report (English MD)](NATGhost_Algorithm_Report.md)

Zamanla proje, temel düzeyde IP ve Port çevirisi (NAT) yapabilen, CLI ekranına sahip kendi halinde basit bir bare-metal router'a (yönlendiriciye) evrildi.

## Temel İçerik
- **Bağımsız Çalışma:** Herhangi bir işletim sistemine (Linux vb.) bağımlı değildir. Doğrudan bare-metal (veya QEMU) üzerinde kendi başına boot edilir.
- **NAT (Ağ Adresi Çevirisi):** Fast-Path mekanizması ile paketlerin IP ve Port değerlerini kendi içinde çevirip Checksum hesaplamalarını donanıma yakın seviyede halleder.
- **ntop Komutu:** Ağa bağlanan LAN cihazlarını NAT tablosundan okuyup basit bir ASCII topoloji haritası çizer.
- **CLI & Telnet:** Kendine ait renkli bir komut satırı arayüzü vardır. 23. porttan Telnet ile dışarıdan da bağlanılabilir.
- **Dosya Sistemsiz Depolama:** FAT32/EXT4 kullanmaz. Ayarları saklamak için `config.img` diskinin 1. sektörüne doğrudan ham (raw) yazma/okuma işlemi yapar (`save` ve `load` komutları ile).

## Çalıştırma (QEMU)

Projeyi denemek isterseniz sisteminizde `gcc`, `make` ve `qemu-system-x86_64` yüklü olmalıdır.

```bash
make slirp
```

Bu komut:
1. Kaynak kodları (`nostdlib` formatında) derler.
2. Ayarların tutulacağı 1MB'lık `config.img` sanal diskini oluşturur (eğer henüz yoksa).
3. QEMU'yu başlatır ve Telnet için 23. portu bilgisayarınızın 2323. portuna bağlar.

Sistem açıldığında `help` yazarak kullanabileceğiniz komutları (örn: `ifconfig`, `ntop`, `clear`, `ls`, `exit`) görebilirsiniz. Dilerseniz kendi terminalinizden `telnet 127.0.0.1 2323` yazarak router'a uzaktan da bağlanabilirsiniz.
