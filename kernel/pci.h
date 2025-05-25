#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI fonksiyonları
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_read_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_num);
void     pci_scan(void);

// VGA'ya mesaj yazmak istersen dışarı açılabilir (isteğe bağlı)
void vga_write_string(const char* msg, int offset);

#endif
