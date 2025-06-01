#include "common.h"
#include "serial.h"
#include "pci.h"
#include "e1000.h"
#include "task.h"

/* pci_find_e1000() – pci.c’de eklediğin fonksiyon */
extern uint32_t pci_find_e1000(void);

void kmain(void)
{
    serial_init();
    kprint("=== BurstKernel starting ===\n");

    uint32_t bar0 = pci_find_e1000();
    if (!bar0) { kprint("NIC not found!\n"); while(1); }

    e1000_init(bar0);
    kprint("E1000 init OK\n");

    fetch_and_execute_tasks();

    kprint("Loop forever…\n");
    for (;;);
}
