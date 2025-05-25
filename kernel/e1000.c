#include "e1000.h"
#include <stdint.h>
#include <string.h>

struct e1000_tx_desc tx_desc[NUM_TX_DESC] __attribute__((aligned(16)));
uint8_t tx_buffers[NUM_TX_DESC][TX_BUFFER_SIZE] __attribute__((aligned(16)));
volatile uint32_t *e1000_mmio = (volatile uint32_t *)0xF0000000; // MMIO base, PCI'den keşfedilmiş

void e1000_send(void *packet, size_t len) {
    uint32_t tail = e1000_mmio[E1000_TDT / 4];

    // Packet'i buffer'a kopyala
    memcpy(tx_buffers[tail], packet, len);

    tx_desc[tail].buffer_addr = (uint64_t)(uintptr_t)&tx_buffers[tail];
    tx_desc[tail].length = len;
    tx_desc[tail].cmd = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    tx_desc[tail].status = 0;

    // Tail'i ilerlet
    tail = (tail + 1) % NUM_TX_DESC;
    e1000_mmio[E1000_TDT / 4] = tail;

    // Eğer istersen burada polling ile gönderimin tamamlandığını bekleyebilirsin
    // while (!(tx_desc[tail].status & E1000_TX_STATUS_DD));
}
#include "e1000.h"

// Çok basit örnek: UDP packet forge edip e1000 üzerinden gönder
void forge_and_send_udp(uint8_t *src_ip, uint8_t *src_mac,
                        uint8_t *dst_ip, uint8_t *dst_mac,
                        uint16_t dst_port) {
    uint8_t packet[128];  // sade örnek için küçük buffer
    int offset = 0;

    // Ethernet Header (14 byte)
    memcpy(packet + offset, dst_mac, 6); offset += 6;
    memcpy(packet + offset, src_mac, 6); offset += 6;
    packet[offset++] = 0x08;  // Type: IPv4
    packet[offset++] = 0x00;

    // IP, UDP başlıkları ve payload burada olurdu...
    // Şimdilik dummy payload: sadece UDP gibi görünmesi için
    for (int i = 0; i < 32; i++)
        packet[offset++] = 0x41;  // 'A'

    // Toplam uzunluğu ver
    e1000_send(packet, offset);
}

