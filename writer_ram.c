/* writer_ram.c  –  VM görev bloklarını /dev/shm/shrmem dosyasına yazar */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>

#define BACKING_SIZE   (128 * 1024 * 1024)   /* 128 MiB */
#define PORT_SPACE   65536
#define SHARE_NAME   "/shrmem"

typedef struct {
    int32_t ID;
    int32_t task_count;
    int32_t task_list[];    /* flexible array */
} memory_block;

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <vm_count>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int vm_count = atoi(argv[1]);
    if (vm_count <= 0) { fputs("invalid vm_count\n", stderr); return -1; }

    int base_task  = PORT_SPACE / vm_count;   /* herkesin garantisi    */
    int remainder  = PORT_SPACE % vm_count;   /* ilk <remainder> VM fazladan alır */

    /* Her blokta max (base_task+1) port olabilir → blok boyutu buna göre */
    size_t block_size = sizeof(memory_block) + (base_task+1) * sizeof(int32_t);
    size_t total_size = block_size * vm_count;

    int fd = shm_open(SHARE_NAME, O_CREAT | O_RDWR , 0666);
    if (fd == -1) { perror("shm_open"); return -1; }

    if (ftruncate(fd, BACKING_SIZE) == -1) { perror("ftruncate"); return -1; }

    void *mem = mmap(NULL, total_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) { perror("mmap"); return -1; }

    int current = 0;
    for (int i = 0; i < vm_count; ++i) {
        int this_task = base_task + (i < remainder);   /* artanı paylaştır */
        memory_block *blk = (memory_block *)((char*)mem + i * block_size);
        blk->ID          = i;
        blk->task_count  = this_task;
        for (int j = 0; j < this_task; ++j)
            blk->task_list[j] = current++;             /* 0-65535 artan port */
        printf("VM[%d] written (%d ports)\n", i, this_task);
    }

    printf("✓ Shared memory ready at /dev/shm%s (%zu bytes)\n",
           SHARE_NAME, total_size);
    munmap(mem, total_size);
    close(fd);
    return 0;
}
