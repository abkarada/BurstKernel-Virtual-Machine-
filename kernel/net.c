#include "net.h"
#include <string.h>

/* Sabit hedef   –  NAT testi için kendi IP/port’unu koyabilirsin   */
static const uint8_t DST_IP[4]  = {192, 168, 0, 1};
static const uint8_t DST_MAC[6] = {0xff,0xff,0xff,0xff,0xff,0xff}; /* broadcast */
static const uint16_t DST_PORT  = 55555;

/* Ethernet + sahte IPv4 + UDP başlık = 42 bayt                               *
 * Demoda checksum doğruluğu önemsenmedi; NAT açmak için yeterli payload gider */
static void forge_udp_packet(uint8_t *buf,
                             const uint8_t *src_mac,
                             const uint8_t *src_ip,
                             uint16_t src_port,
                             uint16_t dst_port,
                             uint16_t payload_id)
{
    int off = 0;

    /* Ethernet ---------------------------------------------------- */
    memcpy(buf+off, DST_MAC, 6);        off += 6;
    memcpy(buf+off, src_mac, 6);        off += 6;
    buf[off++] = 0x08; buf[off++] = 0x00;        /* EtherType = IPv4 */

    /* IPv4 (minimal, checksum = 0) -------------------------------- */
    buf[off++] = 0x45;                  /* ver=4, ihl=5 */
    buf[off++] = 0x00;                  /* TOS          */
    uint16_t total_len = 20+8+2;        /* IP+UDP+payload(2B) */
    buf[off++] = total_len >> 8;
    buf[off++] = total_len & 0xFF;
    buf[off++] = 0x00; buf[off++] = 0x00;        /* ID      */
    buf[off++] = 0x40; buf[off++] = 0x00;        /* flags+frag */
    buf[off++] = 64;                    /* TTL     */
    buf[off++] = 17;                    /* Proto=UDP */
    buf[off++] = 0x00; buf[off++] = 0x00;        /* checksum=0 */

    /* Src IP */
    memcpy(buf+off, src_ip, 4);         off += 4;
    /* Dst IP */
    memcpy(buf+off, DST_IP, 4);         off += 4;

    /* UDP --------------------------------------------------------- */
    buf[off++] = src_port >> 8;
    buf[off++] = src_port & 0xFF;
    buf[off++] = dst_port >> 8;
    buf[off++] = dst_port & 0xFF;
    uint16_t udp_len = 8+2;
    buf[off++] = udp_len >> 8;
    buf[off++] = udp_len & 0xFF;
    buf[off++] = 0x00; buf[off++] = 0x00;        /* checksum=0 */

    /* Payload: görev ID (2 bayt) ---------------------------------- */
    buf[off++] = payload_id >> 8;
    buf[off++] = payload_id & 0xFF;
}

/* VM öntanımlı parametreleri ver */
void init_vm_info(vm_info_t *vm, uint8_t vm_id)
{
    /* MAC = 02:00:00:00:00:ID  (rastgele demo) */
    vm->mac[0] = 0x02; vm->mac[1] = 0x00; vm->mac[2] = 0x00;
    vm->mac[3] = 0x00; vm->mac[4] = 0x00; vm->mac[5] = vm_id;

    /* IP = 10.0.0.ID  */
    vm->ip[0] = 10; vm->ip[1] = 0; vm->ip[2] = 0; vm->ip[3] = vm_id;

    /* Görev listesi: 0-64 arası port no’su gibi; örnek */
    for (int i = 0; i < TASK_SIZE; ++i)
        vm->task_list[i] = 40000 + i;   /* 40000-40064 */
}

/* 65 port’a UDP burst at – düşük seviyede e1000 kullanır */
void send_burst(const vm_info_t *vm)
{
    uint8_t pkt[128];

    for (int i = 0; i < TASK_SIZE; ++i) {
        forge_udp_packet(pkt,
                         vm->mac,
                         vm->ip,
                         /* src_port = */ vm->task_list[i],
                         /* dst_port = */ DST_PORT,
                         /* payload ID */ vm->task_list[i]);

        e1000_send(pkt, 42);            /* toplam 42 baytlık çerçeve */
    }
}
