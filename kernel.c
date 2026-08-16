#include <stdint.h>
#include <stddef.h>

/* ── VGA mini-log (64-bit uyumlu) ─────────────────────────*/
static volatile uint16_t *vga = (uint16_t*)0xB8000; 
static uint16_t cursor = 0;
static uint8_t current_color = 0x0F; // Default: White on Black

void set_color(uint8_t fg, uint8_t bg) {
    current_color = (bg << 4) | (fg & 0x0F);
}

static void serial_write(char a);

void puts(const char *s) {
    while (*s) {
        serial_write(*s);
        if (*s == '\n') {
            cursor = (cursor / 80 + 1) * 80;
        } else if (*s == '\b') {
            if (cursor > 0) {
                cursor--;
                vga[cursor] = (uint16_t)(' ') | (current_color << 8);
            }
        } else {
            vga[cursor++] = (uint16_t)(*s) | (current_color << 8);
        }
        s++;
        if (cursor >= 80 * 25) {
            // Scroll (simple reset for now)
            for (int i = 0; i < 80*25; i++) vga[i] = 0x0F00;
            cursor = 0;
        }
    }
}

/* ── IO Yardımcıları ─────────────────────────────────────*/
void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

/* ── Serial Port Yardımcıları (COM1) ──────────────────────*/
static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);    // Disable all interrupts
    outb(0x3F8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(0x3F8 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(0x3F8 + 1, 0x00);    //                  (hi byte)
    outb(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(0x3F8 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static inline int serial_is_transmit_empty() {
    return inb(0x3F8 + 5) & 0x20;
}

static void serial_write(char a) {
    while (serial_is_transmit_empty() == 0);
    outb(0x3F8, a);
}

void puts_serial(const char *s) {
    while (*s) {
        serial_write(*s++);
    }
}

extern void idt_init(void);
extern void keyboard_init(void);
extern void mm_init(void);
#include "lwip/init.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/ip.h"
#include "lwip/timeouts.h"
#include "e1000.h"

extern err_t ethernetif_init(struct netif *netif);
extern void ethernetif_input(struct netif *netif, const uint8_t *data, uint16_t len);

static struct netif main_netif;

/* ── Yönlendirme Tablosu (Routing Table) ──────────────────*/
struct route_entry {
    uint32_t dest_ip;
    uint32_t netmask;
    uint8_t next_hop_mac[6];
};

#define MAX_ROUTES 16
struct route_entry routing_table[MAX_ROUTES];
int num_routes = 0;

void add_route(uint32_t dest_ip, uint32_t netmask, const uint8_t *mac) {
    if (num_routes < MAX_ROUTES) {
        routing_table[num_routes].dest_ip = dest_ip;
        routing_table[num_routes].netmask = netmask;
        for (int i = 0; i < 6; i++) {
            routing_table[num_routes].next_hop_mac[i] = mac[i];
        }
        num_routes++;
    }
}

struct route_entry *route_lookup(uint32_t ip) {
    for (int i = 0; i < num_routes; i++) {
        if ((ip & routing_table[i].netmask) == (routing_table[i].dest_ip & routing_table[i].netmask)) {
            return &routing_table[i];
        }
    }
    return NULL;
}

static uint16_t ip_checksum(const uint8_t *buf, int len) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)buf;
    for (int i = 0; i < len / 2; i++) {
        sum += ptr[i];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

#include "nat.h"

/* ── Fast-Path (Zero-Copy) Paket Yakalayıcı ───────────────*/
void nk_fastpath_rx_callback(int dev_idx, const uint8_t *data, uint16_t len) {
    if (len < 34) return;
    
    uint16_t eth_type = (data[12] << 8) | data[13];
    if (eth_type == 0x0800) { // IPv4 Packet
        uint32_t dest_ip = *(uint32_t *)(data + 30);
        uint32_t my_ip = 0x0F02000A; // 10.0.2.15
        uint8_t *pkt = (uint8_t *)data; // Cast const away to modify RX buffer in-place
        
        // WAN Inbound (dev_idx == 0) NAT check
        if (dev_idx == 0 && dest_ip == my_ip) {
            if (nat_inbound(pkt, len)) {
                // Packet translated to LAN IP! Update dest_ip to route it.
                dest_ip = *(uint32_t *)(pkt + 30);
                puts_serial("NAT Inbound Translated!\n");
            } else {
                // Not a NAT session, packet is for us (e.g. Telnet). Pass to lwIP.
                ethernetif_input(&main_netif, data, len);
                return;
            }
        }
        else if (dest_ip == my_ip) {
            // From LAN to us (e.g. Gateway ping/Telnet)
            ethernetif_input(&main_netif, data, len);
            return;
        }
        
        // LAN Outbound (dev_idx == 1) NAT check
        if (dev_idx == 1) {
            // SNAT from LAN to WAN
            if (nat_outbound(pkt, len)) {
                puts_serial("NAT Outbound Translated!\n");
            }
        }
        
        // Routing Candidate!
        struct route_entry *route = route_lookup(dest_ip);
        if (route) {
            // 1. Decrement TTL (Byte 22)
            if (pkt[22] <= 1) {
                return; 
            }
            pkt[22] -= 1;
            
            // 2. Recompute Checksum (Incremental is better, but doing full for safety now)
            pkt[24] = 0; pkt[25] = 0;
            int ihl = (pkt[14] & 0x0F) * 4;
            uint16_t new_csum = ip_checksum(pkt + 14, ihl);
            *(uint16_t *)(pkt + 24) = new_csum;
            
            // 3. Update MAC Addresses
            for (int i = 0; i < 6; i++) {
                pkt[i] = route->next_hop_mac[i]; // Dest MAC
                pkt[i+6] = e1000_devs[dev_idx].mac[i]; // Source MAC (our MAC)
            }
            
            puts_serial("Fast-Path Route: Packet Forwarded (Zero-Copy)!\n");
            
            // 4. Send directly (Zero-Copy)
            e1000_send_packet(dev_idx, pkt, len);
            return;
        }
    }
    
    // Not IPv4 or No Route -> pass to lwIP (ARP, ICMP for us, etc.)
    ethernetif_input(&main_netif, data, len);
}

uint32_t sys_now(void) {
    static uint32_t ms = 0;
    static uint32_t counter = 0;
    if (counter++ > 10000) {
        ms += 10;
        counter = 0;
    }
    return ms;
}

/* ── kernel main (64-bit) ────────────────────────────────*/
void kmain(void) {
    __asm__("cli");
    
    serial_init();

    // Clear screen
    for (int i = 0; i < 80*25; i++) vga[i] = 0x0F00;
    cursor = 0;

    puts("BurstKernel Network OS Unikernel Booted!\n");
    puts("Architecture: x86_64 (64-bit Long Mode)\n\n");

    puts_serial("Network OS Unikernel Booted!\n");
    puts_serial("Architecture: x86_64 (64-bit Long Mode)\n");

    mm_init();
    idt_init();
    keyboard_init();
    
    lwip_init();
    puts("lwIP Initialized.\n");
    nat_init();

    // 6. Init E1000 and setup fastpath callback
    e1000_init(nk_fastpath_rx_callback);
    
    ip4_addr_t ip, netmask, gw;
    IP4_ADDR(&ip, 10, 0, 2, 15);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 10, 0, 2, 2);
    
    netif_add(&main_netif, &ip, &netmask, &gw, NULL, ethernetif_init, netif_input);
    netif_set_default(&main_netif);
    netif_set_up(&main_netif);
    
    extern void telnetd_init(void);
    telnetd_init();
    
    puts("Network Interface 'en0' is UP. IP: 10.0.2.15\n");
    puts("Sending ARP Request to Gateway 10.0.2.2...\n");
    etharp_request(&main_netif, &gw);
    
    __asm__("sti");

    // --- Fast-Path Yönlendirme (Zero-Copy) Testi ---
    puts("Setting up Routing Table...\n");
    uint32_t target_net = 0x0001A8C0; // 192.168.1.0 in x86 LE
    uint32_t route_mask = 0x00FFFFFF; // 255.255.255.0 in x86 LE
    uint8_t next_hop_mac[6] = {0x52, 0x54, 0x00, 0x99, 0x99, 0x99};
    add_route(target_net, route_mask, next_hop_mac);
    
    puts("Simulating incoming packet to 192.168.1.10...\n");
    uint8_t dummy_pkt[64] = {
        // Ethernet Header
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56, // Dest MAC (Us)
        0x52, 0x54, 0x00, 0xAA, 0xBB, 0xCC, // Source MAC (Sender)
        0x08, 0x00,                         // EtherType (IPv4)
        // IPv4 Header
        0x45, 0x00, 0x00, 0x2E,             // Version/IHL, TOS, Total Length
        0x00, 0x00, 0x00, 0x00,             // ID, Flags/Frag
        0x40, 0x01, 0x00, 0x00,             // TTL (64), Protocol (ICMP), Checksum (Dummy)
        0x0A, 0x00, 0x02, 0x02,             // Source IP: 10.0.2.2
        0xC0, 0xA8, 0x01, 0x0A,             // Dest IP: 192.168.1.10
        // Payload (Dummy)
        0x00, 0x01, 0x02, 0x03
    };
    
    nk_fastpath_rx_callback(0, dummy_pkt, 64);
    // -----------------------------------------------

    for(;;) {
        sys_check_timeouts();
        e1000_poll();
        // __asm__("hlt"); // Wait for interrupt (disabled for polling test)
    }
}
