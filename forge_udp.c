/* forge_udp.c  —  minimal, libc-siz UDP/IP paket oluşturucu            */
/* x86 little-endian, freestanding kernel ortamı                         */
/* derle:  gcc -ffreestanding -fno-stack-protector -nostdlib -c forge_udp.c */


#include <stddef.h>   /* size_t için */


typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;


#define UDP_PROTO   17      /* RFC 768 */

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

    /* --------------- UDP checksum (pseudo) */
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

/* ---------------------- Örnek kullanım ------------------------------- */
/*
static uint8_t pkt[1500];

void send_test_packet(void)
{
    size_t len = forge_udp_packet(pkt,
                                  0x0A000001,   // 10.0.0.1
                                  55555,
                                  0xC0A80101,   // 192.168.1.1
                                  12345,
                                  (uint8_t *)"NATGhost", 8);

    e1000_tx(pkt, len);  // kendi NIC sürücündeki TX çağrısı
}
*/
-
