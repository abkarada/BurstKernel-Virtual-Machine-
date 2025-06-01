#include "common.h"
#include "pci.h"
#include "e1000.h"
#include "net.h"

void kmain(void)
{
    serial_init();               /* debug için                 */
    kprint("=== BurstKernel starting ===\n");

    /* VGA “Welcome” yazısı */
    const char *msg = "Welcome to BurstKernel!";
    char *vga = (char*)0xB8000;
    for (int i=0; msg[i]; ++i) { vga[i*2] = msg[i]; vga[i*2+1] = 0x0F; }

    /*------------- PCI tarayıp E1000’i aç ----------------------*/
    uint8_t bus, slot, func;
    if (pci_find_device(PCI_VENDOR_INTEL, PCI_DEVICE_E1000,
                        &bus, &slot, &func) == 0) {
        uint32_t bar0 = pci_read_bar(bus, slot, func, 0);
        e1000_init(bar0);
        kprint("E1000 init OK\n");
    } else {
        kprint("E1000 not found - halt\n");
        while (1);
    }

    /*------------- VM infosu ve burst --------------------------*/
    vm_info_t vm;
    init_vm_info(&vm, /*vm_id=*/3);
    send_burst(&vm);

    kprint("Burst sent!\n");
    while (1);
}
