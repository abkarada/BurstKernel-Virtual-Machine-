/*───────────────────────────────────────────────────────────
 *  pci.c   –   Basit PCI erişim katmanı (x86, I/O port yöntemi)
 *───────────────────────────────────────────────────────────*/
#include "pci.h"

/*────────────────────  Yardımcı I/O makroları  ─────────────*/
static inline void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*────────────────────  Config alanı okuma  ─────────────────*/

/* 32-bit okuma */
uint32_t pci_config_read32(uint8_t bus,
                           uint8_t slot,
                           uint8_t func,
                           uint8_t offset)
{
    /* Mechanism #1: 31. bit enable, ardından bus/slot/func/offset */
    uint32_t address = (1U << 31)            |
                       ((uint32_t)bus  << 16)|
                       ((uint32_t)slot << 11)|
                       ((uint32_t)func << 8) |
                       (offset & 0xFC);

    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

/* 16-bit okuma (cfg-space word) */
uint16_t pci_config_read16(uint8_t bus,
                           uint8_t slot,
                           uint8_t func,
                           uint8_t offset)
{
    uint32_t data = pci_config_read32(bus, slot, func, offset);
    return (data >> ((offset & 2) * 8)) & 0xFFFF;
}

/* BAR okuma – alt 4 bit maskelenir; sadece MMIO adresi döner */
uint32_t pci_read_bar(uint8_t bus,
                      uint8_t slot,
                      uint8_t func,
                      uint8_t bar_num)
{
    if (bar_num > 5) return 0;

    uint32_t raw = pci_config_read32(bus,
                                     slot,
                                     func,
                                     PCI_BAR0_OFFSET + bar_num * 4);
    return raw & ~0xF;          /* alt nibble: tip / enable bitleri */
}

/* Belirli aygıtı ara; bulunduysa konumu döndür (0 = başarı) */
int pci_find_device(uint16_t vendor,
                    uint16_t device,
                    uint8_t *out_bus,
                    uint8_t *out_slot,
                    uint8_t *out_func)
{
    for (uint8_t bus = 0; bus < 256; ++bus)
        for (uint8_t slot = 0; slot < 32; ++slot)
            for (uint8_t func = 0; func < 8; ++func) {

                uint16_t v = pci_config_read16(bus, slot, func,
                                               PCI_VENDOR_ID_OFFSET);
                if (v == 0xFFFF) continue;   /* aygıt yok → sonraki */

                uint16_t d = pci_config_read16(bus, slot, func,
                                               PCI_DEVICE_ID_OFFSET);

                if (v == vendor && d == device) {
                    if (out_bus)  *out_bus  = bus;
                    if (out_slot) *out_slot = slot;
                    if (out_func) *out_func = func;
                    return 0;                 /* başarı */
                }
            }
    return -1;                                /* bulunamadı */
}

/* (isteğe bağlı) özet dump – VGA / COM1’e yazdırmak için */
void pci_print_summary(void)
{
    /* Basit örnek: seri porta hexdump atmak istiyorsanız
       kendi serial_putchar / printf fonksiyonunuzu çağırın. */
}

