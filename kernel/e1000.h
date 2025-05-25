#ifndef E1000_H
#define E1000_H
#define E1000_TDT  0x3818
#define E1000_TDH  0x3810
#define E1000_TCTL 0x0400
#define E1000_TDBAL 0x3800
#define E1000_TDBAH 0x3804
#define E1000_TDLEN 0x3808

#define E1000_TX_CMD_EOP  (1 << 0)
#define E1000_TX_CMD_IFCS (1 << 1)
#define E1000_TX_CMD_RS   (1 << 3)
#define E1000_TX_STATUS_DD (1 << 0)

#include <stdint.h>

#define NUM_TX_DESC 64
#define TX_BUFFER_SIZE 2048

void forge_and_send_udp(uint8_t *src_ip, uint8_t *src_mac,
                        uint8_t *dst_ip, uint8_t *dst_mac,
                        uint16_t dst_port);

// TX descriptor formatı — Intel E1000 datasheet'e göre
struct e1000_tx_desc {
    uint64_t buffer_addr;   // Veri buffer’ının fiziksel adresi
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

#endif
