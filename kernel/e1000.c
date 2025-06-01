/*───────────────────────────────────────────────────────────
 *  e1000.c  –  Minimal TX-only E1000 sürücüsü
 *───────────────────────────────────────────────────────────*/
#include "e1000.h"
#include <string.h>             /* memcpy için */

static volatile uint32_t *mmio; /* PCI katmanından gelecek */
static struct e1000_tx_desc tx_desc[NUM_TX_DESC] __attribute__((aligned(16)));
static uint8_t tx_buf[NUM_TX_DESC][TX_BUFFER_SIZE] __attribute__((aligned(16)));

/* Basit fiziksel <-> sanal eşlemesi yoksa phys_to_virt() ekleyin */
#define VIRT_2_PHYS(v) ((uint32_t)(uintptr_t)(v))

/*––––  Sürücü başlatma  –––––*/
void e1000_init(uint32_t mmio_base)
{
    mmio = (volatile uint32_t*)mmio_base;

    /* 1) Descriptor ringini temizle */
    memset(tx_desc, 0, sizeof(tx_desc));
    for (int i = 0; i < NUM_TX_DESC; ++i) {
        tx_desc[i].buffer_addr = VIRT_2_PHYS(tx_buf[i]);
        tx_desc[i].status      = E1000_TX_STATUS_DD;  /* boş */
    }

    /* 2) Donanıma ring adresini bildir */
    mmio[E1000_TDBAL/4] = VIRT_2_PHYS(tx_desc) & 0xFFFFFFFC;
    mmio[E1000_TDBAH/4] = 0x0;                /* 32-bit adresleme */
    mmio[E1000_TDLEN/4] = sizeof(tx_desc);

    mmio[E1000_TDH/4] = 0;
    mmio[E1000_TDT/4] = 0;

    /* 3) TCTL  –  transmitter ON */
    uint32_t tctl = E1000_TCTL_EN | E1000_TCTL_PSP |
                    (0x10 << E1000_TCTL_CT_SHIFT)   |   /* CT = 16 */
                    (0x40 << E1000_TCTL_COLD_SHIFT);    /* COLD = 64 */
    mmio[E1000_TCTL/4] = tctl;

    /* 4) IPG (rekomende: 0x0060200) */
    mmio[E1000_TIPG/4] = 0x0060200;
}

/*––––  Tek paket gönder  –––––*/
void e1000_send(void *packet, uint16_t len)
{
    uint32_t tail = mmio[E1000_TDT/4];

    /* Buffer kopyala */
    memcpy(tx_buf[tail], packet, len);

    tx_desc[tail].length = len;
    tx_desc[tail].cmd    = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    tx_desc[tail].status = 0;               /* DD bayrağını temizle */

    /* Kuyruk ilerlet */
    tail = (tail + 1) % NUM_TX_DESC;
    mmio[E1000_TDT/4] = tail;

    /* Opsiyonel: bitene dek döngü                           *
     * while (!(tx_desc[tail].status & E1000_TX_STATUS_DD)); */
}

/*––––  Örnek: sahte UDP forge  –––––*/
void forge_and_send_udp(uint8_t *src_ip, uint8_t *src_mac,
                        uint8_t *dst_ip, uint8_t *dst_mac,
                        uint16_t dst_port)
{
    uint8_t pkt[128];
    int off = 0;

    /* Ethernet header */
    memcpy(pkt + off, dst_mac, 6); off += 6;
    memcpy(pkt + off, src_mac, 6); off += 6;
    pkt[off++] = 0x08; pkt[off++] = 0x00;       /* EtherType = IPv4 */

    /* (IP + UDP başlıkları atlandı) */
    for (int i = 0; i < 32; ++i) pkt[off++] = 'A';

    e1000_send(pkt, off);
}

