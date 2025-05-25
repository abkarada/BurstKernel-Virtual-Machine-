#include "net.h"
#include "e1000.h"  // forge_and_send_udp() fonksiyonu için

void init_vm_info(vm_info_t *vm, uint8_t vm_id) {
    vm->vm_id = vm_id;

    // IP = 10.20.0.(vm_id + 1)
    vm->ip[0] = 10;
    vm->ip[1] = 20;
    vm->ip[2] = 0;
    vm->ip[3] = vm_id + 1;

    // MAC = 02:AA:00:00:XX:YY
    vm->mac[0] = 0x02;
    vm->mac[1] = 0xAA;
    vm->mac[2] = 0x00;
    vm->mac[3] = 0x00;
    vm->mac[4] = (vm_id >> 4) & 0xFF;
    vm->mac[5] = vm_id & 0x0F;

    // Görev listesi
    vm->task_list = (uint8_t *)(SHARED_MEM_BASE + vm_id * TASK_SIZE);
}

void send_burst(vm_info_t *vm) {
    uint8_t dst_ip[4]  = {203, 0, 113, 1};    // dummy hedef IP
    uint8_t dst_mac[6] = {0xde, 0xad, 0xbe, 0xef, 0x00, 0x01}; // dummy hedef MAC

    for (int i = 0; i < TASK_SIZE; i++) {
        uint16_t port = vm->task_list[i];
        forge_and_send_udp(vm->ip, vm->mac, dst_ip, dst_mac, port);
    }
}

// Kendi memcpy fonksiyonun
void *memcpy(void *dest, const void *src, unsigned int len) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (unsigned int i = 0; i < len; i++) {
        d[i] = s[i];
    }
    return dest;
}
