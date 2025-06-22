/*  ──────────────────────────────────────────────────────────
 *  kernel.c – Bare-metal E1000 TX test çekirdeği (32-bit)
 *  derle:  gcc -m32 -ffreestanding -c kernel.c -o kernel.o
 *  ──────────────────────────────────────────────────────────*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;

#define UDP_PROTO 17 /* RFC 768 */

/* ── I/O port makroları ─────────────────────────────────── */
static inline void outl(uint16_t port, uint32_t val)
{ __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port)); }

static inline uint32_t inl(uint16_t port)
{ uint32_t v; __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(port)); return v; }

static inline uint8_t inb(uint16_t p)
{ uint8_t v; __asm__ volatile("inb  %1,%0":"=a"(v):"Nd"(p)); return v; }

static inline void outb(uint16_t port, uint8_t val)
{ __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port)); }

/* ── Basit bellek yardımcıları (freestanding) ───────────── */
void *memset(void *s, int c, uint32_t n)
{ uint8_t *p = s; while (n--) *p++ = (uint8_t)c; return s; }

void *memcpy(void *d, const void *s, uint32_t n)
{ uint8_t *dst = d; const uint8_t *src = s; while (n--) *dst++ = *src++; return d; }

/* ---------- Küçük ↔ Büyük endian dönüşümleri (libc yok) -------------- */
static inline uint16_t htons(uint16_t v)
{
    return (uint16_t)((v << 8) | (v >> 8));
}
static inline uint32_t htonl(uint32_t v)
{
    return ((v & 0x000000FFU) << 24) |
           ((v & 0x0000FF00U) <<  8) |
           ((v & 0x00FF0000U) >>  8) |
           ((v & 0xFF000000U) >> 24);
}
/* ntohs/ntohl eşdeğer — UDP/IP forge’de sadece gidiş yönü lazım */

/* ---------------------- Basit memcpy’ler ----------------------------- */
static void *memcpy8(void *dst, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

/* ---------------------- Internet checksum ---------------------------- */
static uint16_t checksum(const void *data, uint32_t len)
{
    const uint16_t *buf = (const uint16_t *)data;
    uint32_t sum = 0;

    while (len > 1) { sum += *buf++; len -= 2; }
    if (len) sum += *(const uint8_t *)buf;

    /* katla, tersle */
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)(~sum);
}
/* ---------------------- Başlık yapıları ------------------------------ */
typedef struct __attribute__((packed))
{
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t  zero;
    uint8_t  proto;
    uint16_t udp_len;
} pseudo_header;

typedef struct __attribute__((packed))
{
    uint8_t  ver_ihl;      /* 0x45 => IPv4, 5*4 = 20 B header */
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t hdr_cksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} ip_header;

typedef struct __attribute__((packed))
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t cksum;
} udp_header;

/* ---------------------- Ana forge fonksiyonu ------------------------- */
/* packet_buf: 1500 B (MTU) civarı global/statik bellek ayır             */
/* Dönen değer: paket bayt uzunluğu                                      */
size_t forge_udp_packet(uint8_t  *packet_buf,
                        uint32_t  src_ip,
                        uint16_t  src_port,
                        uint32_t  dst_ip,
                        uint16_t  dst_port,
                        const uint8_t *payload,
                        uint32_t  payload_len)
{
    /* Konumla */
    ip_header  *ip  = (ip_header *) packet_buf;
    udp_header *udp = (udp_header *)(packet_buf + sizeof(ip_header));
    uint8_t    *data= packet_buf + sizeof(ip_header) + sizeof(udp_header);

    /* ---------------- UDP ---------------- */
    udp->src_port = htons(src_port);
    udp->dst_port = htons(dst_port);
    udp->len      = htons((uint16_t)(sizeof(udp_header) + payload_len));
    udp->cksum    = 0;            /* geçici */

    /* ---------------- Payload ------------ */
    if (payload_len)
        memcpy8(data, payload, payload_len);

    /* ---------------- IP ----------------- */
    ip->ver_ihl   = 0x45;
    ip->tos       = 0;
    ip->total_len = htons((uint16_t)(sizeof(ip_header) + sizeof(udp_header) + payload_len));
    ip->id        = htons(0x0000);        /* sabit veya artırılabilir      */
    ip->frag_off  = htons(0x0000);        /* DF/fragment yok               */
    ip->ttl       = 64;                   /* NAT’lar için yeterli          */
    ip->proto     = UDP_PROTO;
    ip->hdr_cksum = 0;                    /* önce 0                         */
    ip->src_ip    = htonl(src_ip);
    ip->dst_ip    = htonl(dst_ip);

    /* IP checksum */
    ip->hdr_cksum = checksum(ip, sizeof(ip_header));

    /* ---------------- UDP checksum (pseudo) */
    pseudo_header ph;
    ph.src_addr = htonl(src_ip);
    ph.dst_addr = htonl(dst_ip);
    ph.zero     = 0;
    ph.proto    = UDP_PROTO;
    ph.udp_len  = udp->len;               /* zaten network byte-order’de   */

    /* geçici alan: pseudo + udp + payload */
    const uint32_t pseudo_len = sizeof(pseudo_header) +
                                sizeof(udp_header) + payload_len;
    uint8_t temp[pseudo_len];             /* 1-2 KiB stack’in yeter        */

    memcpy8(temp, &ph, sizeof(pseudo_header));
    memcpy8(temp + sizeof(pseudo_header), udp,
            sizeof(udp_header) + payload_len);

    uint16_t u_ck = checksum(temp, pseudo_len);
    udp->cksum = u_ck ? u_ck : 0xFFFF;    /* RFC: checksum=0 ⇒ “hesaplanmadı” */

    return sizeof(ip_header) + sizeof(udp_header) + payload_len;
}

/* MAC’leri elle ver veya  ff:ff:ff:ff:ff:ff  broadcast kullan */
static size_t forge_eth_udp(uint8_t *p,
                             const uint8_t dst_mac[6],
                             const uint8_t src_mac[6],
                             uint32_t sip, uint16_t sport,
                             uint32_t dip, uint16_t dport,
                             const uint8_t *pl, uint32_t plen)
{
    /* 1) Ethernet header */
    memcpy8(p, dst_mac, 6);
    memcpy8(p+6, src_mac, 6);
    p[12]=0x08; p[13]=0x00;            /* EtherType IPv4 */

    /* 2) IP+UDP forge çağrısı hemen arkasından */
    size_t l = forge_udp_packet(p+14, sip,sport,dip,dport,pl,plen);

    /* 3) Min-Ethernet boyu (60) yakala */
    size_t frame_len = 14 + l;
    if (frame_len < 60) {
        memset(p+frame_len, 0, 60-frame_len);
        frame_len = 60;
    }
    return frame_len;
}

/* ── VGA debug çıktısı ──────────────────────────────────── */
volatile uint8_t *vga = (volatile uint8_t *)0xB8000;
uint16_t cursor_pos = 0;

void ft_putchar(char c)
{ vga[cursor_pos++] = c; vga[cursor_pos++] = 0x07; }

void ft_putstr(const char *s)
{ while (*s) ft_putchar(*s++); }

void ft_puthex(uint32_t v)
{
    const char *hex = "0123456789ABCDEF";
    ft_putstr("0x");
    for (int i = 7; i >= 0; --i)
        ft_putchar(hex[(v >> (i * 4)) & 0xF]);
}
/* ── PCI temel sabitleri ────────────────────────────────── */
#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC
#define PCI_VENDOR   0x00
#define PCI_DEVICE   0x02

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off)
{
    uint32_t addr = (1u << 31) | (bus << 16) | (dev << 11)
                  | (fn << 8) | (off & 0xFC);
    outl(PCI_CFG_ADDR, addr);
    return inl(PCI_CFG_DATA);
}

uint16_t pci_read16(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off)
{ uint32_t d = pci_read32(bus, dev, fn, off); return (d >> ((off & 2) * 8)) & 0xFFFF; }

void  pci_write16(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off, uint16_t v)
{
    uint32_t addr = (1u << 31) | (bus << 16) | (dev << 11)
                  | (fn << 8) | (off & 0xFC);
    outl(PCI_CFG_ADDR, addr);
    outl(PCI_CFG_DATA + (off & 2), v);
}

/* ── E1000 sabitleri ───────────────────────────────────── */
#define INTEL_VID 0x8086
#define E1000_DID 0x100E
#define E1000_IMC 0x00D8   /* Interrupt Mask Clear */
#define E1000_ICR   0x00C0   /* Interrupt Cause Read */

#define NUM_TX_DESC   8
#define TX_BUF_SIZE   2048
#define V2P(v)  ((uint32_t)(uintptr_t)(v))

/* MMIO offset’leri */
#define TDBAL   0x03800
#define TDBAH   0x03804
#define TDLEN   0x03808
#define TDH     0x03810
#define TDT     0x03818
#define TCTL    0x00400
#define TIPG    0x00410

#define TCTL_EN     (1<<1)
#define TCTL_PSP    (1<<3)
#define TCTL_CT(x)  ((x)<<4)
#define TCTL_COLD(x)((x)<<12)

/* TX descriptor yapısı */
struct tx_desc {
    uint64_t buf;
    uint16_t len;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((aligned(16)));

static struct tx_desc tx_ring[NUM_TX_DESC] __attribute__((aligned(16)));
static uint8_t tx_buf[NUM_TX_DESC][TX_BUF_SIZE] __attribute__((aligned(16)));

/* ── Ring başlat ────────────────────────────────────────── */
void tx_ring_init(void)
{
    for (int i = 0; i < NUM_TX_DESC; ++i) {
        memset(&tx_ring[i], 0, sizeof(struct tx_desc));
        tx_ring[i].buf    = V2P(tx_buf[i]);
        tx_ring[i].status = 1;          /* DD = boş */
    }
}

/* ── E1000’i TX için hazırla ───────────────────────────── */
void e1000_init_tx(volatile uint32_t *mmio)
{
    mmio[TDBAL/4] = V2P(tx_ring);
    mmio[TDBAH/4] = 0;
    mmio[TDLEN/4] = NUM_TX_DESC * sizeof(struct tx_desc);
    mmio[TDH  /4] = 0;
    mmio[TDT  /4] = 0;

    mmio[TCTL/4] = TCTL_EN | TCTL_PSP | TCTL_CT(0x10) | TCTL_COLD(0x40);
    mmio[TIPG/4] = 0x0060200;

    mmio[E1000_IMC / 4] = 0xFFFFFFFF;   /* bütün kesmeleri kapat */
    (void)mmio[E1000_ICR/4];          /* read-to-clear */
}

/* ── Tek paket gönder ──────────────────────────────────── */
void e1000_send(const void *pkt, uint16_t len, volatile uint32_t *mmio)
{
    uint32_t cur = mmio[TDT/4];
    if (len < 60) {                      /* min Ethernet boyu */
        memcpy(tx_buf[cur], pkt, len);
        memset(tx_buf[cur]+len, 0, 60-len);
        len = 60;
    } else {
        memcpy(tx_buf[cur], pkt, len);
    }

    tx_ring[cur].len    = len;
    tx_ring[cur].cmd    = 0b10011;        /* EOP|IFCS|RS */
    tx_ring[cur].status = 0;

    mmio[TDT/4] = (cur + 1) % NUM_TX_DESC;

    while (!(tx_ring[cur].status & 1)) ; /* DD bekle */
    ft_putstr("TX OK\n");
}

/* ── PCI’de E1000 bul, MMIO’sunu döndür ────────────────── */
volatile uint32_t *find_e1000(void)
{
    for (uint8_t b=0;b<256;b++)
        for (uint8_t d=0;d<32;d++) {
            uint16_t vid = pci_read16(b,d,0,PCI_VENDOR);
            uint16_t did = pci_read16(b,d,0,PCI_DEVICE);
            if (vid==INTEL_VID && did==E1000_DID) {
                /* Bus-master + mem enable */
                uint16_t cmd = pci_read16(b,d,0,0x04);
                cmd |= (1<<2)|(1<<1);
                pci_write16(b,d,0,0x04,cmd);

                uint32_t bar0 = pci_read32(b,d,0,0x10);
                return (volatile uint32_t*)(bar0 & 0xFFFFFFF0);
            }
        }
    return 0;
}
/* ── Çekirdek giriş noktası ────────────────────────────── */
void kmain(void)
{
__asm__ volatile ("cli");          /* IF = 0              */

/* 0x70 portu: bit7=1 → NMI MASK */
uint8_t prev = inb(0x70);
outb(0x70, prev | 0x80);           /* NMI kapandı         */

outb(0x21, 0xFF);                  /* PIC master mask     */
outb(0xA1, 0xFF);     
 
    ft_putstr("BurstLab!\n");

    volatile uint32_t *mmio = find_e1000();
    if (!mmio) {
        ft_putstr("NIC not found\n");
        for(;;)__asm__("hlt");
    }
    ft_putstr("E1000 detected\n");

    tx_ring_init();
    e1000_init_tx(mmio);

	static uint8_t pkt[1500] __attribute__((aligned(16)));

	static const uint8_t bcast[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	static const uint8_t mymac[6] = {0x02,0x00,0x00,0x00,0x00,0x01};

	size_t len = forge_eth_udp(pkt,
                            bcast, mymac,
                            0x0A000001, 55555,
                            0xC0A80101, 12345,
                            (uint8_t*)"BURST", 5);

	e1000_send(pkt, len, mmio);

	__asm__ volatile ("sti");   /* tüm IRQ’ları kapat */


    for(;;)__asm__("hlt");
}
