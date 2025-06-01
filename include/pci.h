/*───────────────────────────────────────────────────────────
 *  pci.h  –  Minimal PCI access layer (x86, I/O-port mechanism #1)
 *             BurstKernel / NATGhost project
 *───────────────────────────────────────────────────────────*/
#ifndef PCI_H_
#define PCI_H_

#include <stdint.h>

/* ──────────────────  I/O port tabanlı erişim  ────────────────── */
#define PCI_CONFIG_ADDRESS   0xCF8      /* 32-bit addr register  */
#define PCI_CONFIG_DATA      0xCFC      /* 32-bit data register  */

/* ──────────────────  CFG-space sabit ofsetler  ───────────────── */
#define PCI_VENDOR_ID_OFFSET     0x00   /* 16-bit */
#define PCI_DEVICE_ID_OFFSET     0x02   /* 16-bit */
#define PCI_HEADER_TYPE_OFFSET   0x0E   /* 8-bit  */
#define PCI_BAR0_OFFSET          0x10   /* BAR0-BAR5: 0x10…0x24 */

/* ─────  Intel 8254x “E1000” (en yaygın virt sürümü: 0x100E)  ─── */
#define PCI_VENDOR_INTEL         0x8086
#define PCI_DEVICE_E1000_82540   0x100E /* (VirtualBox, QEMU)   */
/* İhtiyaç hâlinde başka DID’leri ekle:
   #define PCI_DEVICE_E1000_82545   0x100F
   #define PCI_DEVICE_E1000_82574   0x10D3
*/

/* ─────────────────────────  Prototipler  ─────────────────────── */

/* Konfigürasyon alanı okuma (Mechanism #1) */
uint32_t pci_config_read32(uint8_t bus,
                           uint8_t slot,
                           uint8_t func,
                           uint8_t offset);

uint16_t pci_config_read16(uint8_t bus,
                           uint8_t slot,
                           uint8_t func,
                           uint8_t offset);

/* Belirtilen BAR’ın fiziksel MMIO adresini getirir (alt 4 bit maskeli) */
uint32_t pci_read_bar(uint8_t bus,
                      uint8_t slot,
                      uint8_t func,
                      uint8_t bar_num);

/* Verilen Vendor ID & Device ID’ye sahip aygıtı arar.                     *
 * Bulunursa bus / slot / func koordinatlarını doldurur, 0 döner.          *
 * Bulunamazsa −1 döner.                                                   */
int pci_find_device(uint16_t vendor,
                    uint16_t device,
                    uint8_t *out_bus,
                    uint8_t *out_slot,
                    uint8_t *out_func);

/* E1000 MMIO taban adresini tek çağrıda elde etmek için küçük kısayol
 * (bulunamazsa 0 döner)                                                   */
uint32_t pci_find_e1000(void);

/* (Opsiyonel) Tüm aygıtları özet geç; VGA veya seri log için kullanılır.  */
void pci_print_summary(void);

#endif /* PCI_H_ */

