/*───────────────────────────────────────────────────────────
 *  e1000.h  –  Intel 82540EM (E1000) asgari TX sürücüsü
 *───────────────────────────────────────────────────────────*/
#ifndef _E1000_H_
#define _E1000_H_

#include <stdint.h>

/*–– Konfigürasyon –––––––––––––––––––––––––––––––––––––––––*/
#define NUM_TX_DESC     8                   /* güç-on test için yeterli  */
#define TX_BUFFER_SIZE  2048

/*–– Descriptor tipleri ––––––––––––––––––––––––––––––––––––*/
struct e1000_tx_desc {
    uint64_t buffer_addr;   /* fiziksel adres */
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed));

/* TX command bitleri */
#define E1000_TX_CMD_EOP   0x01
#define E1000_TX_CMD_IFCS  0x02
#define E1000_TX_CMD_RS    0x08

/* TX status bitleri */
#define E1000_TX_STATUS_DD 0x01

/*–– MMIO register offset’leri (datasheet §13) ––––––––––––*/
#define E1000_TDBAL   0x03800
#define E1000_TDBAH   0x03804
#define E1000_TDLEN   0x03808
#define E1000_TDH     0x03810
#define E1000_TDT     0x03818
#define E1000_TCTL    0x00400
#define E1000_TIPG    0x00410

/* TCTL bit alanları */
#define E1000_TCTL_EN     (1 << 1)
#define E1000_TCTL_PSP    (1 << 3)
#define E1000_TCTL_CT_SHIFT   4
#define E1000_TCTL_COLD_SHIFT 12

/* Prototipler */
void e1000_init(uint32_t mmio_base);
void e1000_send(void *packet, uint16_t len);

#endif /* _E1000_H_ */

