#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "netif/ppp/pppoe.h"

#include "e1000.h"

extern void puts(const char *s);

err_t ethernetif_linkoutput(struct netif *netif, struct pbuf *p) {
    // Send a packet via E1000
    // Copy pbuf into a contiguous buffer if necessary
    uint8_t buffer[2048];
    uint16_t copied = pbuf_copy_partial(p, buffer, p->tot_len, 0);
    
    e1000_send_packet(0, buffer, copied);
    return ERR_OK;
}

err_t ethernetif_init(struct netif *netif) {
    netif->linkoutput = ethernetif_linkoutput;
    netif->output = etharp_output;
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    
    // QEMU default E1000 MAC: 52:54:00:12:34:56
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif->hwaddr[0] = 0x52;
    netif->hwaddr[1] = 0x54;
    netif->hwaddr[2] = 0x00;
    netif->hwaddr[3] = 0x12;
    netif->hwaddr[4] = 0x34;
    netif->hwaddr[5] = 0x56;
    
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    
    return ERR_OK;
}

void ethernetif_input(struct netif *netif, const uint8_t *data, uint16_t len) {
    struct pbuf *p, *q;
    
    // Allocate a pbuf chain of pbufs from the pool
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p != NULL) {
        // Copy data to pbuf
        pbuf_take(p, data, len);
        
        // Pass to network interface
        if (netif->input(p, netif) != ERR_OK) {
            pbuf_free(p);
        }
    }
}
