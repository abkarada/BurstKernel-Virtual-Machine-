#ifndef NAT_H
#define NAT_H

#include <stdint.h>
#include <stddef.h>

#define MAX_NAT_ENTRIES 2048
#define NAT_TIMEOUT 300 // example arbitrary ticks

// Protocols
#define NAT_PROTO_TCP 6
#define NAT_PROTO_UDP 17

struct nat_entry {
    uint32_t internal_ip;
    uint16_t internal_port;
    uint16_t external_port;
    uint8_t protocol;
    uint32_t last_active;
    uint8_t in_use;
};

// Global NAT table
extern struct nat_entry nat_table[MAX_NAT_ENTRIES];

// Init
void nat_init(void);

// Outbound (LAN -> WAN)
// Returns 1 if NAT translated, 0 otherwise
int nat_outbound(uint8_t *pkt, uint16_t len);

// Inbound (WAN -> LAN)
// Returns 1 if NAT matched and translated, 0 otherwise
int nat_inbound(uint8_t *pkt, uint16_t len);

// Get internal IP mapped to this external port + protocol
uint32_t nat_get_internal_ip(uint16_t external_port, uint8_t protocol);

#endif
