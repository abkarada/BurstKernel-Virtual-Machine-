#include "nat.h"
#include <string.h>

struct nat_entry nat_table[MAX_NAT_ENTRIES];
static uint16_t next_port = 50000;
static uint32_t nat_ticks = 0;

void nat_init(void) {
    memset(nat_table, 0, sizeof(nat_table));
    next_port = 50000;
    nat_ticks = 0;
}

// Simple LCG pseudo-random for port allocation if needed, or just sequential
static uint16_t allocate_port(void) {
    uint16_t p = next_port++;
    if (next_port > 65000) next_port = 50000;
    return p;
}

static void csum_replace2(uint16_t *sum, uint16_t old_val, uint16_t new_val) {
    uint32_t s = ~(*sum) & 0xFFFF;
    s += (~old_val & 0xFFFF) + new_val;
    s = (s >> 16) + (s & 0xFFFF);
    s += (s >> 16);
    *sum = ~s;
}

static void csum_replace4(uint16_t *sum, uint32_t old_val, uint32_t new_val) {
    csum_replace2(sum, old_val >> 16, new_val >> 16);
    csum_replace2(sum, old_val & 0xFFFF, new_val & 0xFFFF);
}

int nat_outbound(uint8_t *pkt, uint16_t len) {
    if (len < 34) return 0;

    uint8_t protocol = pkt[23];
    if (protocol != NAT_PROTO_TCP && protocol != NAT_PROTO_UDP) return 0;

    uint32_t src_ip = *(uint32_t *)(pkt + 26);
    uint32_t dst_ip = *(uint32_t *)(pkt + 30);
    
    int ihl = (pkt[14] & 0x0F) * 4;
    uint16_t *src_port_ptr = (uint16_t *)(pkt + 14 + ihl);
    uint16_t *dst_port_ptr = (uint16_t *)(pkt + 14 + ihl + 2);
    
    uint16_t src_port = *src_port_ptr;
    
    // Find existing session
    int entry_idx = -1;
    for (int i = 0; i < MAX_NAT_ENTRIES; i++) {
        if (nat_table[i].in_use && nat_table[i].internal_ip == src_ip &&
            nat_table[i].internal_port == src_port && nat_table[i].protocol == protocol) {
            entry_idx = i;
            break;
        }
    }
    
    // Create new session
    if (entry_idx == -1) {
        for (int i = 0; i < MAX_NAT_ENTRIES; i++) {
            if (!nat_table[i].in_use) {
                entry_idx = i;
                nat_table[i].internal_ip = src_ip;
                nat_table[i].internal_port = src_port;
                nat_table[i].external_port = allocate_port();
                // To account for endianness since allocate_port is host order (little)
                // wait, ports in packet are big-endian. allocate_port gives little endian? 
                // Let's just swap bytes of allocated port.
                uint16_t ep = nat_table[i].external_port;
                nat_table[i].external_port = (ep << 8) | (ep >> 8); 
                
                nat_table[i].protocol = protocol;
                nat_table[i].in_use = 1;
                break;
            }
        }
    }
    
    if (entry_idx == -1) return 0; // NAT table full
    
    nat_table[entry_idx].last_active = nat_ticks++;
    
    // Perform Translation
    uint32_t wan_ip = 0x0F02000A; // 10.0.2.15 (Little Endian representation)
    uint16_t ext_port = nat_table[entry_idx].external_port;
    
    uint16_t *ip_csum = (uint16_t *)(pkt + 24);
    csum_replace4(ip_csum, src_ip, wan_ip);
    *(uint32_t *)(pkt + 26) = wan_ip;
    
    uint16_t *l4_csum = NULL;
    if (protocol == NAT_PROTO_TCP) {
        l4_csum = (uint16_t *)(pkt + 14 + ihl + 16);
    } else if (protocol == NAT_PROTO_UDP) {
        l4_csum = (uint16_t *)(pkt + 14 + ihl + 6);
    }
    
    if (l4_csum && *l4_csum != 0) {
        csum_replace4(l4_csum, src_ip, wan_ip);
        csum_replace2(l4_csum, src_port, ext_port);
    }
    
    *src_port_ptr = ext_port;
    
    return 1;
}

int nat_inbound(uint8_t *pkt, uint16_t len) {
    if (len < 34) return 0;

    uint8_t protocol = pkt[23];
    if (protocol != NAT_PROTO_TCP && protocol != NAT_PROTO_UDP) return 0;

    uint32_t dst_ip = *(uint32_t *)(pkt + 30);
    uint32_t wan_ip = 0x0F02000A; // 10.0.2.15
    if (dst_ip != wan_ip) return 0;

    int ihl = (pkt[14] & 0x0F) * 4;
    uint16_t *dst_port_ptr = (uint16_t *)(pkt + 14 + ihl + 2);
    uint16_t dst_port = *dst_port_ptr;
    
    // Find matching session
    int entry_idx = -1;
    for (int i = 0; i < MAX_NAT_ENTRIES; i++) {
        if (nat_table[i].in_use && nat_table[i].external_port == dst_port && nat_table[i].protocol == protocol) {
            entry_idx = i;
            break;
        }
    }
    
    if (entry_idx == -1) return 0; // No mapping, drop or ignore
    
    nat_table[entry_idx].last_active = nat_ticks++;
    
    // Translate back to LAN
    uint32_t lan_ip = nat_table[entry_idx].internal_ip;
    uint16_t lan_port = nat_table[entry_idx].internal_port;
    
    uint16_t *ip_csum = (uint16_t *)(pkt + 24);
    csum_replace4(ip_csum, dst_ip, lan_ip);
    *(uint32_t *)(pkt + 30) = lan_ip;
    
    uint16_t *l4_csum = NULL;
    if (protocol == NAT_PROTO_TCP) {
        l4_csum = (uint16_t *)(pkt + 14 + ihl + 16);
    } else if (protocol == NAT_PROTO_UDP) {
        l4_csum = (uint16_t *)(pkt + 14 + ihl + 6);
    }
    
    if (l4_csum && *l4_csum != 0) {
        csum_replace4(l4_csum, dst_ip, lan_ip);
        csum_replace2(l4_csum, dst_port, lan_port);
    }
    
    *dst_port_ptr = lan_port;
    
    return 1;
}

uint32_t nat_get_internal_ip(uint16_t external_port, uint8_t protocol) {
    for (int i = 0; i < MAX_NAT_ENTRIES; i++) {
        if (nat_table[i].in_use && nat_table[i].external_port == external_port && nat_table[i].protocol == protocol) {
            return nat_table[i].internal_ip;
        }
    }
    return 0;
}
