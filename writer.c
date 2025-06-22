#define PCI_CONFIG_ADDRESS	0xCF8 /* 32-bit address register */
#define PCI_CONFIG_DATA		0xCFC /*32-bit data register */
#define PCI_VENDOR_ID_OFFSET  0x00
#define PCI_DEVICE_ID_OFFSET  0x02



#define NUM_TX_DESC	8
#define TX_BUFFER_SIZE	2048
#define VIRT_2_PHYS(v) ((uint32_t)(uintptr_t)(v))


#define INTEL_VID  0x8086
#define E1000_DID  0x100E

#define E1000_TDBAL   0x03800
#define E1000_TDBAH   0x03804
#define E1000_TDLEN   0x03808
#define E1000_TDH     0x03810
#define E1000_TDT     0x03818
#define E1000_TCTL    0x00400
#define E1000_TIPG    0x00410

#define E1000_TCTL_EN     (1 << 1)
#define E1000_TCTL_PSP    (1 << 3)
#define E1000_TCTL_CT_SHIFT   4
#define E1000_TCTL_COLD_SHIFT 12

#define E1000_TX_CMD_EOP   (1 << 0)  // End of Packet
#define E1000_TX_CMD_IFCS  (1 << 1)  // Insert FCS (CRC)
#define E1000_TX_CMD_RS    (1 << 3)  // Report Status
#define E1000_TX_STATUS_DD (1 << 0)  // Descriptor Done


static struct e1000_tx_desc tx_desc_ring[NUM_TX_DESC];

static uint8_t tx_buf[NUM_TX_DESC][TX_BUFFER_SIZE] __attribute__((aligned(16)));

typedef struct {
	uint32_t base;
	uint32_t size;
	bool	is_io;
	bool	is_64bit;

}pci_bar_t;

struct e1000_tx_desc {
	uint64_t buffer_addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
} __attribute__((packed, aligned(16)));


void tx_ring_init()
{
	for(int i = 0; i < NUM_TX_DESC; ++i)
	{
		tx_desc_ring[i].buffer_addr = VIRT_2_PHYS(tx_buf[i]);
		tx_desc_ring[i].status = 0x1;
	}
}

void e1000_init_tx(volatile uint32_t *mmio)
{
    // 1) TX descriptor ring adresini bildir
    mmio[E1000_TDBAL / 4] = VIRT_2_PHYS(tx_desc_ring) & 0xFFFFFFFF;
    mmio[E1000_TDBAH / 4] = 0x0; // 32-bit adresleme (QEMU için yeterli)

    // 2) Boyutunu bildir (kaç byte?)
    mmio[E1000_TDLEN / 4] = NUM_TX_DESC * sizeof(struct e1000_tx_desc); // 8 × 16 = 128

    // 3) Baş ve kuyruk pointer'ları sıfırla
    mmio[E1000_TDH / 4] = 0;
    mmio[E1000_TDT / 4] = 0;

    // 4) Göndericiyi aktif et
    mmio[E1000_TCTL / 4] =
        E1000_TCTL_EN |
        E1000_TCTL_PSP |
        (0x10 << E1000_TCTL_CT_SHIFT) |     // Collision threshold
        (0x40 << E1000_TCTL_COLD_SHIFT);    // Collision distance

    // 5) Inter-packet gap ayarı (tavsiye edilen)
    mmio[E1000_TIPG / 4] = 0x0060200;
}
void e1000_send(void *packet, uint16_t len, volatile uint32_t *mmio)
{
    // 1) Şu anki TDT (tail) değerini al
    uint32_t cur = mmio[E1000_TDT / 4];

    // 2) Veriyi buffer'a kopyala
    if (len < 60) len = 60; // Ethernet minimum boyutu (pad gereği)
    memcpy(tx_buf[cur], packet, len);

    // 3) Descriptor alanlarını doldur
    tx_desc_ring[cur].length = len;
    tx_desc_ring[cur].cmd = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    tx_desc_ring[cur].status = 0; // DD bitini temizle

    // 4) TDT'yi bir ileriye al → E1000’e "yeni paket hazır" de
    mmio[E1000_TDT / 4] = (cur + 1) % NUM_TX_DESC;

    // 5) İsteğe bağlı olarak DD biti beklenebilir
    volatile uint32_t spins = 0;
    while (!(tx_desc_ring[cur].status & E1000_TX_STATUS_DD))
        spins++;

    // 6) Log (seri port, VGA vs.)
    kprint("Packet sent, spins = "); kprint_hex(spins); kprint("\n");
}

void *memcpy(void *dest, const void *src, uint32_t len)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    for (uint32_t i = 0; i < len; i++)
        d[i] = s[i];

    return dest;
}


static inline void outl(uint16_t port, uint32_t val)
{
	__asm__ volatile("out %0, %1": : "a"(val), "Nd"(port));

}

static inline uint32_t inl(uint16_t port)
{
	uint32_t ret;
	__asm__ volatile("inl %1, %0" : : "=a"(ret) : "Nd"(port));
	return ret;
}


uint32_t pci_config_read32(uint8_t bus,
			   uint8_t slot,
			   uint8_t func,
			   uint8_t offset)
{
	uint32_t address =	(1U << 31)	      |
				((uint32_t)bus << 16) |
				((uint32_t)slot << 11)|
				((uint32_t)func << 8) |
				(offset & 0xFC);
	outl(PCI_CONFIG_ADDRESS, address);
	return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus,
			   uint8_t slot,
			   uint8_t func,
			   uint8_t offset)
{
	uint32_t data = pci_config_read32(bus, slot, func, offset);
	return (data >> ((offset & 2) * 8)) & 0xFFFF;
}

pci_bar_t pci_read_bar(uint8_t bus, uint8_t dev, uint8_t func, uint8_t bar_index)
{
	pci_bar_t bar = {0};
	uint8_t offset = 0x10 + (bar_index*4);

	uint32_t value = pci_config_read32(bus, dev,
					   func, offset);
	if(value == 0 || value == 0xFFFFFFFF) return bar;

	bar.is_io = value & 0x1;
	if(bar.is_io)
	{
		bar.base = value & 0xFFFFFFFC;
		bar.size = 0;
		bar.is_64bit = false;
		return bar;
	}	

	uint32_t type = (value >> 1) & 0x3;
	bar.is_64bit = (type == 0x2);
	bar.base = value & 0xFFFFFFF0;

	if(bar.is_64bit && bar_index < 5)
	{
		uint32_t upper = pci_config_read32(bus, dev, func, offset + 4);
		bar.base |= ((uint64_t)upper <<32);

	}

	return bar;
}

void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset,uint32_t value)
{
	uint32_t addr = (1U << 31)    |
		        (bus << 16)   |
			(device << 11)|
			(func << 8)   |
			(offset & 0xFC);
	outl(PCI_CONFIG_ADDR, addr);
	outl(PCI_CONFIG_DATA, value);
}

void pci_config_write16(uint8_t bus, uint8_t slot, uint8_t func,
                        uint8_t offset, uint16_t value)
{
    uint32_t addr = (1U << 31) | ((uint32_t)bus  << 16) |
                    ((uint32_t)slot << 11) | ((uint32_t)func << 8) |
                    (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, addr);
    outl(PCI_CONFIG_DATA + (offset & 2), (uint32_t)value);
}


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
                if (v == 0xFFFF) continue;   

                uint16_t d = pci_config_read16(bus, slot, func,
                                               PCI_DEVICE_ID_OFFSET);

                if (v == vendor && d == device) {
                    if (out_bus)  *out_bus  = bus;
                    if (out_slot) *out_slot = slot;
                    if (out_func) *out_func = func;
                    return 0;               
                }
            }
    return -1;                                
}

void pci_enable_busmaster(uint8_t bus, uint8_t slot, uint8_t func)
{
    uint16_t cmd = pci_config_read16(bus, slot, func, 0x04);   
    cmd |= 0x0004  | 0x0002;
    pci_config_write16(bus, slot, func, 0x04, cmd);
}

uint32_t pci_find_e1000(void)
{
    uint8_t bus, slot, func;
         if (pci_find_device(INTEL_VID, E1000_DID,
                        &bus, &slot, &func) == 0)
    {
        /* 1) Bus-master + memory-space enable         */
        pci_enable_busmaster(bus, slot, func);

        /* 2) BAR0 yalnızca fiziksel MMIO adresi       */
        return pci_read_bar(bus, slot, func, 0);
    }
    return 0;   /* bulunamadı → 0 döndür */
 }
uint32_t find_e1000_mmio_base() {
    for (uint8_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint32_t id = pci_config_read(bus, dev, 0, 0x00);
            if (id == 0xFFFFFFFF) continue;

            uint16_t vendor = id & 0xFFFF;
            uint16_t device = (id >> 16) & 0xFFFF;

            // Intel E1000 için vendor = 0x8086, device = 0x100E
            if (vendor == 0x8086 && device == 0x100E) {
                // BAR0 (0x10)
                uint32_t bar0 = pci_config_read(bus, dev, 0, 0x10);
                uint32_t mmio_base = bar0 & 0xFFFFFFF0;

                // Command register: 0x04
                uint16_t command = pci_config_read(bus, dev, 0, 0x04) & 0xFFFF;
                command |= (1 << 1); // Memory Space Enable
                command |= (1 << 2); // Bus Master Enable
                pci_config_write(bus, dev, 0, 0x04, command);

                return mmio_base;
            }
        }
    }
    return 0; // bulunamadı
}
