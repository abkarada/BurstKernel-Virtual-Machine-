#include <stdio.h>
#include <stdint.h>

#define MAX_VM 1000
#define TASK_SIZE 65

int main() {
    FILE *f = fopen("taskpool.bin", "wb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    for (int vm = 0; vm < MAX_VM; vm++) {
        for (int i = 0; i < TASK_SIZE; i++) {
            uint8_t port = (vm * TASK_SIZE + i) % 255 + 1;
            fwrite(&port, 1, 1, f);
        }
    }

    fclose(f);
    printf("[+] taskpool.bin generated.\n");
    return 0;
}
