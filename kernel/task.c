#include "task.h"
#include "e1000.h"
#include "serial.h"
#include <string.h>

/* Basit MAC/IP şablonları – diler­sen gerçek değerlerle değiştir. */
static const uint8_t SRC_MAC[6] = { 0x02,0x00,0x00,0x00,0x00, VM_ID };
static const uint8_t DST_MAC[6] = { 0xff,0xff,0xff,0xff,0xff,0xff };
static const uint8_t SRC_IP [4] = { 10,0,0,VM_ID };
static const uint8_t DST_IP [4] = { 10,0,0,1 };

#define ETH_TYPE_IPV4 0x0800
#define IP_PROTO_UDP  17

/* basit htons/htonl yardımcıları (little-endian x86) */
static inline uint16_t swap16(uint16_t v){return (v<<8)|(v>>8);}
static inline uint32_t swap32(uint32_t v){return __builtin_bswap32(v);}

/* UDP paketi oluşturup E1000’e gönderir */
static void forge_and_send_udp(uint16_t dst_port)
{
    /* Ethernet(14) + IPv4(20) + UDP(8) + payload(0) */
    uint8_t frame[14+20+8] = {0};
    uint8_t *eth = frame;
    uint8_t *ip  = frame + 14;
    uint8_t *udp = frame + 14 + 20;

    /* Ethernet hdr */
    memcpy(eth, DST_MAC, 6);
    memcpy(eth+6, SRC_MAC, 6);
    *(uint16_t *)(eth+12) = swap16(ETH_TYPE_IPV4);

    /* IPv4 hdr (minimum 20 byte) */
    ip[0]  = 0x45;                    /* ver=4, ihl=5 */
    ip[1]  = 0x00;                    /* DSCP/ECN      */
    *(uint16_t*)(ip+2) = swap16(20+8);/* tot len       */
    *(uint16_t*)(ip+4) = 0;           /* ID            */
    *(uint16_t*)(ip+6) = 0x4000;      /* flags: DF     */
    ip[8]  = 64;                      /* TTL           */
    ip[9]  = IP_PROTO_UDP;
    memcpy(ip+12, SRC_IP, 4);
    memcpy(ip+16, DST_IP, 4);
    /* Basit header checksum – tek uint32 havuzlu paketlerde 0 kabul edebilirsin */

    /* UDP hdr */
    *(uint16_t*)(udp+0) = swap16(12345);      /* src port   */
    *(uint16_t*)(udp+2) = swap16(dst_port);   /* dst port   */
    *(uint16_t*)(udp+4) = swap16(8);          /* len        */
    *(uint16_t*)(udp+6) = 0;                  /* checksum=0 */

    e1000_send(frame, sizeof(frame));   /* e1000.c’de mevcut */
}

void fetch_and_execute_tasks(void)
{
    task_block_t *blk = task_block(VM_ID);

    if (blk->ID != VM_ID || blk->task_count != TASK_COUNT) {
        kprint("Task-block mismatch!\n");
        return;
    }

    kprint("Burst list fetched…\n");
    for (int i = 0; i < TASK_COUNT; ++i)
        forge_and_send_udp((uint16_t)blk->task_list[i]);

    kprint("Burst complete.\n");
}
