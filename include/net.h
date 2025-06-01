#ifndef _NET_H_
#define _NET_H_

#include <stdint.h>
#include "e1000.h"   /* e1000_send() prototipi için */

/* Basit görev listesi */
#define TASK_SIZE 65

typedef struct {
    uint8_t  ip[4];
    uint8_t  mac[6];
    uint16_t task_list[TASK_SIZE];   /* her port burada    */
} vm_info_t;

/* API */
void init_vm_info(vm_info_t *vm, uint8_t vm_id);
void send_burst(const vm_info_t *vm);

#endif

