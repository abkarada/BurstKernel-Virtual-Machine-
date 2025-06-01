/*───────────────────────────────────────────────────────────
 *  pci.h   –   Basit PCI erişim katmanı (x86, I/O port yöntemi)
 *───────────────────────────────────────────────────────────*/
#ifndef _PCI_H_
#define _PCI_H_

#include <stdint.h>

/* I/O port adresleri (PCI Configuration Mechanism #1) */
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

/* CFG-space offset sabitleri (PCI 2.3 spec) */
#define PCI_VENDOR_ID_OFFSET    0x00
#define PCI_DEVICE_ID_OFFSET    0x02
#define PCI_HEADER_TYPE_OFFSET  0x0E
#define PCI_BAR0_OFFSET         0x10   /* BAR0-BAR5 = 0x10 … 0x24 */

/* Intel E1000 (82540EM) tanımlayıcıları */
#define PCI_VENDOR_INTEL   0x8086
#define PCI_DEVICE_E1000   0x100E

/*────────────────────────  Prototipler  ─────────────────────*/
uint32_t pci_config_read32(uint8_t bus,
                           uint8_t slot,
                           uint8_t func,
                           uint8_t offset);

uint16_t pci_config_read16(uint8_t bus,
                           uint8_t slot,
                           uint8_t func,
                           uint8_t offset);

uint32_t pci_read_bar(uint8_t bus,
                      uint8_t slot,
                      uint8_t func,
                      uint8_t bar_num);

int pci_find_device(uint16_t vendor,
                    uint16_t device,
                    uint8_t *out_bus,
                    uint8_t *out_slot,
                    uint8_t *out_func);

void pci_print_summary(void);   /* (isteğe bağlı) VGA/seri log */

#endif /* _PCI_H_ */

