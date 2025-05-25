#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stddef.h>

#define TASK_SIZE 65
#define SHARED_MEM_BASE 0x200000

typedef struct {
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t vm_id;
    uint8_t *task_list;
} vm_info_t;

// ↓↓↓ BUNLARI YAZMAZSAN 'undefined reference' HATASI ALIRSIN
void init_vm_info(vm_info_t *vm, uint8_t vm_id);
void send_burst(vm_info_t *vm);
void *memcpy(void *dest, const void *src, unsigned int len);

#endif
