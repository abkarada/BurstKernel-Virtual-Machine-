#include <stdint.h>
#include "pci.c"

void kmain(void){
	const char *msg = "Welcome to BurstKernel!";
	char *vga = (char *)0xb8000;

	for(int i = 0; msg[i] != '\0'; ++i){
		vga[i * 2] = msg[i];
		vga[i * 2 + 1] = 0x0F;
	}

	pci_scan();
	while(1);
}
