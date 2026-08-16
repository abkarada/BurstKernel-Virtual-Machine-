#include "e1000.h"
#include "mm.h"

extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);
extern void puts(const char *s);

/* ── PCI Registers ── */
#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC
#define INTEL_VID    0x8086
#define E1000_DID    0x100E

/* ── E1000 MMIO Registers ── */
#define E1000_CTRL   0x00000
#define E1000_ICR    0x000C0
#define E1000_IMS    0x000D0
#define E1000_IMC    0x000D8
#define E1000_RCTL   0x00100
#define E1000_TCTL   0x00400
#define E1000_RDBAL  0x02800
#define E1000_RDBAH  0x02804
#define E1000_RDLEN  0x02808
#define E1000_RDH    0x02810
#define E1000_RDT    0x02818
#define E1000_TDBAL  0x03800
#define E1000_TDBAH  0x03804
#define E1000_TDLEN  0x03808
#define E1000_TDH    0x03810
#define E1000_TDT    0x03818

#define RCTL_EN                         (1 << 1)    /* Receiver Enable */
#define RCTL_SBP                        (1 << 2)    /* Store Bad Packets */
#define RCTL_UPE                        (1 << 3)    /* Unicast Promiscuous Enabled */
#define RCTL_MPE                        (1 << 4)    /* Multicast Promiscuous Enabled */
#define RCTL_LPE                        (1 << 5)    /* Long Packet Reception Enable */
#define RCTL_LBM_NONE                   (0 << 6)    /* No Loopback */
#define RCTL_LBM_PHY                    (3 << 6)    /* PHY or external SerDesc loopback */
#define RCTL_RDMTS_HALF                 (0 << 8)    /* Free Buffer Threshold is 1/2 of RDLEN */
#define RCTL_RDMTS_QUARTER              (1 << 8)    /* Free Buffer Threshold is 1/4 of RDLEN */
#define RCTL_RDMTS_EIGHTH               (2 << 8)    /* Free Buffer Threshold is 1/8 of RDLEN */
#define RCTL_MO_36                      (0 << 12)   /* Multicast Offset - bits 47:36 */
#define RCTL_MO_35                      (1 << 12)   /* Multicast Offset - bits 46:35 */
#define RCTL_MO_34                      (2 << 12)   /* Multicast Offset - bits 45:34 */
#define RCTL_MO_32                      (3 << 12)   /* Multicast Offset - bits 43:32 */
#define RCTL_BAM                        (1 << 15)   /* Broadcast Accept Mode */
#define RCTL_VFE                        (1 << 18)   /* VLAN Filter Enable */
#define RCTL_CFIEN                      (1 << 19)   /* Canonical Form Indicator Enable */
#define RCTL_CFI                        (1 << 20)   /* Canonical Form Indicator Bit Value */
#define RCTL_DPF                        (1 << 22)   /* Discard Pause Frames */
#define RCTL_PMCF                       (1 << 23)   /* Pass MAC Control Frames */
#define RCTL_SECRC                      (1 << 26)   /* Strip Ethernet CRC */

#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))

#define TCTL_EN                         (1 << 1)    /* Transmit Enable */
#define TCTL_PSP                        (1 << 3)    /* Pad Short Packets */

struct e1000_rx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t status;
    volatile uint8_t errors;
    volatile uint16_t special;
} __attribute__((packed));

struct e1000_tx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t cso;
    volatile uint8_t cmd;
    volatile uint8_t status;
    volatile uint8_t css;
    volatile uint16_t special;
} __attribute__((packed));

struct e1000_device e1000_devs[MAX_E1000_DEVICES];
int num_e1000_devices = 0;

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ( "inl %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t address = (1u << 31) | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CFG_ADDR, address);
    return inl(PCI_CFG_DATA);
}

static void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t address = (1u << 31) | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CFG_ADDR, address);
    outl(PCI_CFG_DATA, val);
}

static void find_all_e1000(void) {
    num_e1000_devices = 0;
    for(uint16_t bus = 0; bus < 256; bus++) {
        for(uint16_t dev = 0; dev < 32; dev++) {
            uint32_t vendor_device = pci_read(bus, dev, 0, 0);
            if ((vendor_device & 0xFFFF) == INTEL_VID && (vendor_device >> 16) == E1000_DID) {
                // Enable Bus Mastering & Memory Space
                uint32_t cmd = pci_read(bus, dev, 0, 4);
                pci_write(bus, dev, 0, 4, cmd | (1 << 2) | (1 << 1));
                
                uint32_t bar0 = pci_read(bus, dev, 0, 0x10);
                
                if (num_e1000_devices < MAX_E1000_DEVICES) {
                    e1000_devs[num_e1000_devices].mmio = (volatile uint32_t *)(uint64_t)(bar0 & 0xFFFFFFF0);
                    e1000_devs[num_e1000_devices].rx_cur = 0;
                    e1000_devs[num_e1000_devices].tx_cur = 0;
                    num_e1000_devices++;
                }
            }
        }
    }
}

static inline void mmio_write(int dev_idx, uint32_t reg, uint32_t value) {
    e1000_devs[dev_idx].mmio[reg / 4] = value;
}

static inline uint32_t mmio_read(int dev_idx, uint32_t reg) {
    return e1000_devs[dev_idx].mmio[reg / 4];
}

static e1000_rx_callback_t rx_cb = NULL;

void e1000_init(e1000_rx_callback_t cb) {
    rx_cb = cb;
    find_all_e1000();
    
    if (num_e1000_devices == 0) {
        puts("No E1000 NICs Found!\n");
        return;
    }
    
    for (int idx = 0; idx < num_e1000_devices; idx++) {
        // Initialize RX Ring
        struct e1000_device *dev = &e1000_devs[idx];
        dev->rx_ring = (struct e1000_rx_desc *)kmalloc(sizeof(struct e1000_rx_desc) * E1000_RX_RING_SIZE);
        for(int i = 0; i < E1000_RX_RING_SIZE; i++) {
            dev->rx_ring[i].addr = (uint64_t)kmalloc(2048);
            dev->rx_ring[i].status = 0;
        }
        mmio_write(idx, E1000_RDBAL, (uint32_t)((uint64_t)dev->rx_ring & 0xFFFFFFFF));
        mmio_write(idx, E1000_RDBAH, (uint32_t)((uint64_t)dev->rx_ring >> 32));
        mmio_write(idx, E1000_RDLEN, E1000_RX_RING_SIZE * 16);
        mmio_write(idx, E1000_RDH, 0);
        mmio_write(idx, E1000_RDT, E1000_RX_RING_SIZE - 1);
        
        // Enable RX
        mmio_write(idx, E1000_RCTL, RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048);

        // Initialize TX Ring
        dev->tx_ring = (struct e1000_tx_desc *)kmalloc(sizeof(struct e1000_tx_desc) * E1000_TX_RING_SIZE);
        for(int i = 0; i < E1000_TX_RING_SIZE; i++) {
            dev->tx_ring[i].addr = 0;
            dev->tx_ring[i].cmd = 0;
            dev->tx_ring[i].status = 1;
        }
        mmio_write(idx, E1000_TDBAL, (uint32_t)((uint64_t)dev->tx_ring & 0xFFFFFFFF));
        mmio_write(idx, E1000_TDBAH, (uint32_t)((uint64_t)dev->tx_ring >> 32));
        mmio_write(idx, E1000_TDLEN, E1000_TX_RING_SIZE * 16);
        mmio_write(idx, E1000_TDH, 0);
        mmio_write(idx, E1000_TDT, 0);
        
        // Enable TX
        mmio_write(idx, E1000_TCTL, TCTL_EN | TCTL_PSP | (15 << 4) | (0x40 << 12));
        
        // Enable Interrupts
        mmio_write(idx, E1000_IMS, 0x1F6DC);
        mmio_read(idx, E1000_ICR); // Clear pending
        
        // Read MAC Address from EEPROM or RAL/RAH registers
        uint32_t ral = mmio_read(idx, 0x5400); // Receive Address Low
        uint32_t rah = mmio_read(idx, 0x5404); // Receive Address High
        dev->mac[0] = ral & 0xFF;
        dev->mac[1] = (ral >> 8) & 0xFF;
        dev->mac[2] = (ral >> 16) & 0xFF;
        dev->mac[3] = (ral >> 24) & 0xFF;
        dev->mac[4] = rah & 0xFF;
        dev->mac[5] = (rah >> 8) & 0xFF;
    }
    puts("E1000 NICs Initialized.\n");
}

void e1000_send_packet(int dev_idx, const uint8_t *data, uint16_t len) {
    if (dev_idx >= num_e1000_devices) return;
    struct e1000_device *dev = &e1000_devs[dev_idx];
    
    dev->tx_ring[dev->tx_cur].addr = (uint64_t)data;
    dev->tx_ring[dev->tx_cur].length = len;
    dev->tx_ring[dev->tx_cur].cmd = (1 << 3) | (1 << 1) | (1 << 0); // RS | IFCS | EOP
    dev->tx_ring[dev->tx_cur].status = 0;
    
    uint16_t old_cur = dev->tx_cur;
    dev->tx_cur = (dev->tx_cur + 1) % E1000_TX_RING_SIZE;
    mmio_write(dev_idx, E1000_TDT, dev->tx_cur);
    
    // Wait for completion with timeout
    int timeout = 1000000;
    while(!(dev->tx_ring[old_cur].status & 0x01) && timeout > 0) {
        timeout--;
    }
}

void e1000_handle_interrupt(void) {
    for (int idx = 0; idx < num_e1000_devices; idx++) {
        struct e1000_device *dev = &e1000_devs[idx];
        uint32_t icr = mmio_read(idx, E1000_ICR);
        
        while((dev->rx_ring[dev->rx_cur].status & 0x01)) {
            if (rx_cb) {
                rx_cb(idx, (const uint8_t *)dev->rx_ring[dev->rx_cur].addr, dev->rx_ring[dev->rx_cur].length);
            }
            
            // Mark descriptor ready
            dev->rx_ring[dev->rx_cur].status = 0;
            mmio_write(idx, E1000_RDT, dev->rx_cur);
            dev->rx_cur = (dev->rx_cur + 1) % E1000_RX_RING_SIZE;
        }
    }
    
    // Send EOI to PICs
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void e1000_poll(void) {
    for (int idx = 0; idx < num_e1000_devices; idx++) {
        struct e1000_device *dev = &e1000_devs[idx];
        
        while((dev->rx_ring[dev->rx_cur].status & 0x01)) {
            if (rx_cb) {
                rx_cb(idx, (const uint8_t *)dev->rx_ring[dev->rx_cur].addr, dev->rx_ring[dev->rx_cur].length);
            }
            
            dev->rx_ring[dev->rx_cur].status = 0;
            mmio_write(idx, E1000_RDT, dev->rx_cur);
            dev->rx_cur = (dev->rx_cur + 1) % E1000_RX_RING_SIZE;
        }
    }
}
