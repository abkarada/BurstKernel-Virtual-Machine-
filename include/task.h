/* task.h – merkezi görev havuzu */
#pragma once
#include <stdint.h>

#define TASK_COUNT      65
#define TASK_POOL_BASE  0x00100000U    /* writer_ram.c’in yazdığı phys addr */

/* Her VM’i derlerken:  -DVM_ID=<n>  bayrağı ver. */
#ifndef VM_ID
#  define VM_ID 0
#endif

typedef struct {
    int32_t ID;
    int32_t task_count;
    int32_t task_list[TASK_COUNT];
} task_block_t;

/* Havuzdaki i’nci blok adresi */
static inline task_block_t *task_block(uint32_t i)
{
    return (task_block_t *)(TASK_POOL_BASE + i * sizeof(task_block_t));
}

/*───────────────────────────────────────────────────────────
 * Çekirdeğin kmain() içinde çağırdığı yüksek seviye rutin  */
void fetch_and_execute_tasks(void);
