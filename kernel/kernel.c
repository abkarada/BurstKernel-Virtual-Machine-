#include <stdint.h>
#include "pci.h"
#include "net.h"

void kmain(void){
	const char *msg = "Welcome to BurstKernel!";
	char *vga = (char *)0xb8000;

	vm_info_t vm;
	uint8_t vm_id = 3; // örnek: gerçek sistemden verilecek
	

	for(int i = 0; msg[i] != '\0'; ++i){
		vga[i * 2] = msg[i];
		vga[i * 2 + 1] = 0x0F;
	}

	pci_scan();
	
	    init_vm_info(&vm, vm_id);
		send_burst(&vm);    // 65 portu hedefe burst at

 // IP'yi yazdır
  /*  kprint("IP: ");
    print_ip(vm.ip);    // özel bir ip yazdırma fonksiyonu yazabilirsin
    kprint("\n");

    // MAC yazdır
    kprint("MAC: ");
    print_mac(vm.mac);
    kprint("\n");

    // Görev listesini yazdır
    for (int i = 0; i < TASK_SIZE; i++) {
        char buf[10];
        itoa(vm.task_list[i], buf);  // basit sayıyı stringe çeviren fonksiyon
        kprint("Task[");
        itoa(i, buf);
        kprint(buf);
        kprint("] = ");
        kprint(buf);
        kprint("\n");
    }
*/
    	while(1);

	}
