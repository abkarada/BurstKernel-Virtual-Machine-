/*─────────────────────────────────────────────────────────────
  kernel.c – Bare-metal (Multiboot) E1000 24 000-Port UDP Burst
  derle:
      gcc -m32 -ffreestanding -nostdlib -std=gnu99 \
          -fno-stack-protector -c kernel.c -o kernel.o
─────────────────────────────────────────────────────────────*/

#include <stdint.h>
#include <stddef.h>

/* ── KULLANICI AYARI ───────────────────────────────────────*/
#define SRC_IP         0x0A00020F   /* 10.0.2.15            */
#define SRC_PORT       55555
#define DST_IP         0xC0A80101   /* 192.168.1.1          */
#define DST_PORT_BASE  10000
#define BURST_COUNT    24000        /* 24 k paket           */

/* ── Sabitler & Ring boyutu ───────────────────────────────*/
#define UDP_PROTO      17
#define RING_SZ        1024         /* === NUM_TX_DESC ===  */
#define TX_BUF_SIZE    2048

/* ── PCI/E1000 register’leri (kısaltılmış) ────────────────*/
#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC
#define INTEL_VID    0x8086
#define E1000_DID    0x100E
#define TDBAL 0x03800
#define TDBAH 0x03804
#define TDLEN 0x03808
#define TDH   0x03810
#define TDT   0x03818
#define TCTL  0x00400
#define TIPG  0x00410
#define TCTL_EN   (1<<1)
#define TCTL_PSP  (1<<3)
#define TCTL_CT(x)   ((x)<<4)
#define TCTL_COLD(x) ((x)<<12)

/* ── IO & yardımcılar ─────────────────────────────────────*/
static inline void outl(uint16_t p,uint32_t v){__asm__("outl %0,%1"::"a"(v),"Nd"(p));}
static inline uint32_t inl(uint16_t p){uint32_t v;__asm__("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(uint16_t p,uint8_t v){__asm__("outb %0,%1"::"a"(v),"Nd"(p));}

static void *memcpy(void *d,const void *s,uint32_t n){uint8_t*D=d;const uint8_t*S=s;while(n--)*D++=*S++;return d;}
static void *memset(void *d,int c,uint32_t n){uint8_t*D=d;while(n--)*D++=(uint8_t)c;return d;}

static inline uint16_t htons(uint16_t v){return (v<<8)|(v>>8);}
static inline uint32_t htonl(uint32_t v){return (v<<24)|(v>>24)|((v&0xFF00)<<8)|((v&0xFF0000)>>8);}

/* ── İnternet Checksum ────────────────────────────────────*/
static uint16_t cksum(const void *d,uint32_t l){
    const uint16_t *b=d; uint32_t s=0;
    while(l>1){s+=*b++; l-=2;} if(l) s+=*(uint8_t*)b;
    s=(s>>16)+(s&0xFFFF); s+=(s>>16); return (uint16_t)~s;
}

/* ── VGA mini-log ─────────────────────────────────────────*/
static volatile uint8_t *vga=(uint8_t*)0xB8000; static uint16_t cur=0;
static void puts(const char *s){while(*s){vga[cur++]=*s++;vga[cur++]=0x07;if(cur>=80*25*2)cur=0;}}

/* ── TX descriptor & ring ────────────────────────────────*/
struct tx_desc{
    uint64_t buf;
    uint16_t len; uint8_t cso; uint8_t cmd;
    uint8_t  status; uint8_t css; uint16_t special;
}__attribute__((aligned(16)));

static struct tx_desc tx_ring[RING_SZ]              __attribute__((aligned(16)));
static uint8_t        tx_buf [RING_SZ][TX_BUF_SIZE] __attribute__((aligned(16)));
static uint32_t tdt=0;

/* ── UDP/IP/Eth forge – tek kopya ────────────────────────*/
static size_t forge_eth_udp(uint8_t *f,
    const uint8_t dst[6],const uint8_t src[6],
    uint32_t sip,uint16_t sport,uint32_t dip,uint16_t dport,
    const uint8_t *pl,uint32_t plen)
{
    memcpy(f,dst,6); memcpy(f+6,src,6); f[12]=0x08; f[13]=0x00;
    uint8_t *ip=f+14;
    ip[0]=0x45; ip[1]=0;
    uint16_t tot=htons(20+8+plen); memcpy(ip+2,&tot,2);
    ip[4]=ip[5]=ip[6]=ip[7]=0; ip[8]=64; ip[9]=UDP_PROTO;
    ip[10]=ip[11]=0;
    *(uint32_t*)(ip+12)=htonl(sip); *(uint32_t*)(ip+16)=htonl(dip);
    uint8_t *udp=ip+20;
    uint16_t sp=htons(sport),dp=htons(dport),ul=htons(8+plen);
    memcpy(udp,&sp,2); memcpy(udp+2,&dp,2); memcpy(udp+4,&ul,2); udp[6]=udp[7]=0;
    memcpy(udp+8,pl,plen);
    uint16_t ipck=cksum(ip,20); memcpy(ip+10,&ipck,2);
    struct{uint32_t s,d;uint8_t z,p;uint16_t l;}__attribute__((packed)) ph=
        {htonl(sip),htonl(dip),0,UDP_PROTO,ul};
    uint8_t tmp[sizeof(ph)+8+plen];
    memcpy(tmp,&ph,sizeof(ph)); memcpy(tmp+sizeof(ph),udp,8+plen);
    uint16_t uck=cksum(tmp,sizeof(tmp)); if(!uck) uck=0xFFFF;
    memcpy(udp+6,&uck,2);
    size_t fr=14+20+8+plen; if(fr<60){memset(f+fr,0,60-fr); fr=60;}
    return fr;
}

/* ── Ring init + RS bitleri ──────────────────────────────*/
static void ring_init(void){
    for(int i=0;i<RING_SZ;i++){
        tx_ring[i].buf=(uint64_t)(uintptr_t)tx_buf[i];
        tx_ring[i].status=1;
    }
    for(int i=255;i<RING_SZ;i+=256) tx_ring[i].cmd|=(1<<3);
}

/* ── PCI/E1000 yardımcıları ─────────────────────────────*/
static uint32_t pci_r32(uint8_t b,uint8_t d,uint8_t f,uint8_t o){
    uint32_t a=(1u<<31)|(b<<16)|(d<<11)|(f<<8)|(o&0xFC);
    outl(PCI_CFG_ADDR,a); return inl(PCI_CFG_DATA);
}
static uint16_t pci_r16(uint8_t b,uint8_t d,uint8_t f,uint8_t o){
    uint32_t v=pci_r32(b,d,f,o); return (v>>((o&2)*8))&0xFFFF;
}
static void pci_w16(uint8_t b,uint8_t d,uint8_t f,uint8_t o,uint16_t v){
    uint32_t a=(1u<<31)|(b<<16)|(d<<11)|(f<<8)|(o&0xFC);
    outl(PCI_CFG_ADDR,a); outl(PCI_CFG_DATA+(o&2),v);
}
static volatile uint32_t *find_e1000(void){
    for(uint8_t b=0;b<256;b++)for(uint8_t d=0;d<32;d++)
        if(pci_r16(b,d,0,0)==INTEL_VID && pci_r16(b,d,0,2)==E1000_DID){
            uint16_t c=pci_r16(b,d,0,4)|((1<<2)|(1<<1));
            pci_w16(b,d,0,4,c);
            return (volatile uint32_t*)(pci_r32(b,d,0,0x10)&0xFFFFFFF0);
        }
    return 0;
}
static void e1000_tx_init(volatile uint32_t *m){
    m[TDBAL/4]=(uint32_t)(uintptr_t)tx_ring; m[TDBAH/4]=0;
    m[TDLEN/4]=RING_SZ*sizeof(struct tx_desc);
    m[TDH/4]=m[TDT/4]=0;
    m[TCTL/4]=TCTL_EN|TCTL_PSP|TCTL_CT(0x10)|TCTL_COLD(0x40);
    m[TIPG/4]=0x0060200;
}

/* ── Blok gönderim ───────────────────────────────────────*/
static inline int free_desc(uint32_t i){return tx_ring[i].status&1;}
static void send_block(const uint8_t *buf,uint16_t len,volatile uint32_t *m){
    uint32_t n=0;
    while(n<RING_SZ && free_desc(tdt)){
        memcpy(tx_buf[tdt],buf,len);
        tx_ring[tdt].len=len;
        tx_ring[tdt].cmd=0b00000011;      /* EOP|IFCS  */
        tx_ring[tdt].status=0;
        tdt=(tdt+1)&(RING_SZ-1); ++n;
    }
    m[TDT/4]=tdt;
}

/* ── kernel main ─────────────────────────────────────────*/
void kmain(void){
    __asm__("cli");
    outb(0x21,0xFF); outb(0xA1,0xFF);   /* IRQ maskli */
    puts("BurstLab\n");

    volatile uint32_t *mmio=find_e1000();
    if(!mmio){ puts("NIC?"); for(;;)__asm__("hlt"); }
    puts("E1000 OK\n");

    ring_init(); e1000_tx_init(mmio);
    /* --- yüksek hızlı 24k burst --- */
    const uint8_t dst_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
    const uint8_t src_mac[6] = {0x02,0x00,0x00,0x00,0x00,0x01};

    for (uint32_t i = 0; i < BURST_COUNT; ) {

        uint32_t blk = (BURST_COUNT - i > RING_SZ) ? RING_SZ
                                                   : (BURST_COUNT - i);

        for (uint32_t k = 0; k < blk; ++k, ++i) {
            uint16_t dstp = DST_PORT_BASE + i;

            /* forge doğrudan DMA buffer’ına */
            size_t len = forge_eth_udp(
                            tx_buf[tdt],
                            dst_mac, src_mac,
                            SRC_IP, SRC_PORT,
                            DST_IP, dstp,
                            (uint8_t*)"BURST", 5);

            tx_ring[tdt].len    = (uint16_t)len;
            tx_ring[tdt].cmd    = 0b00000011;  /* EOP | IFCS, RS önceden var */
            tx_ring[tdt].status = 0;

            tdt = (tdt + 1) & (RING_SZ - 1);
        }
        /* blok tamam → tek MMIO */
        mmio[TDT/4] = tdt;
    }
    puts("24k burst done\n");

   
     for(;;)__asm__("hlt");
}
