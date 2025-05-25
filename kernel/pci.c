#include <stdint.h>


#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
#define PCI_VENDOR_ID_OFFSET 0x00
#define PCI_DEVICE_ID_OFFSET 0x02
// PCI config alanından 16-bit veri oku

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1U << 31)
                     | ((uint32_t)bus << 16)
                     | ((uint32_t)slot << 11)
                     | ((uint32_t)func << 8)
                     | (offset & 0xFC);
                     
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t data = inl(PCI_CONFIG_DATA);
    
    return (data >> ((offset & 2) * 8)) & 0xFFFF;
}

void vga_write_string(const char* msg, int offset) {
    char* vga = (char*)0xB8000 + offset;
    for (int i = 0; msg[i] != '\0'; i++) {
        vga[i*2] = msg[i];
        vga[i*2+1] = 0x07;
    }
}



uint32_t pci_read_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_num) {
    if (bar_num > 5) return 0;

    uint8_t offset = 0x10 + (bar_num * 4);  // BAR0 = 0x10, BAR1 = 0x14, ...
    
    uint32_t address = (1U << 31)
                     | ((uint32_t)bus << 16)
                     | ((uint32_t)slot << 11)
                     | ((uint32_t)func << 8)
                     | (offset & 0xFC);

    outl(0xCF8, address);
    uint32_t bar = inl(0xCFC);
    
    return bar;
}

void pci_scan(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint16_t vendor = pci_config_read_word(bus, slot, func, PCI_VENDOR_ID_OFFSET);
                if (vendor == 0xFFFF || vendor == 0x0000)
                    continue;

                uint16_t device = pci_config_read_word(bus, slot, func, PCI_DEVICE_ID_OFFSET);

				if (vendor == 0x8086 && device == 0x100E) {
    			uint32_t bar0 = pci_read_bar(bus, slot, func, 0);
    			bar0 &= ~0xF; // MMIO adresi için alt bitleri maskeleriz

    			char *vga = (char*)0xB8000;
   				 vga_write_string("NIC!", 160);

    			// BAR adresini göster (örnek: "BAR=FEC00000")
    			const char *hex = "0123456789ABCDEF";
    			for (int i = 0; i < 8; i++) {
       			uint8_t nibble = (bar0 >> ((7 - i) * 4)) & 0xF;
        		vga[180 + i*2] = hex[nibble];
     		    vga[180 + i*2 + 1] = 0x0E;
    }

    return;
}

            }
        }
    }
}

