#pragma once
#include <stdint.h>
#include <stddef.h>

#define E1000_RX_RING_SIZE 32
#define E1000_TX_RING_SIZE 32

typedef void (*e1000_rx_callback_t)(int dev_idx, const uint8_t *data, uint16_t len);

#define MAX_E1000_DEVICES 4

struct e1000_rx_desc;
struct e1000_tx_desc;

struct e1000_device {
    volatile uint32_t *mmio;
    struct e1000_rx_desc *rx_ring;
    struct e1000_tx_desc *tx_ring;
    uint16_t rx_cur;
    uint16_t tx_cur;
    uint8_t mac[6];
};

extern struct e1000_device e1000_devs[MAX_E1000_DEVICES];
extern int num_e1000_devices;

void e1000_init(e1000_rx_callback_t cb);
void e1000_send_packet(int dev_idx, const uint8_t *data, uint16_t len);
void e1000_handle_interrupt(void);
void e1000_poll(void);
