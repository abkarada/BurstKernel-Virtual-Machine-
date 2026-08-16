#include "mm.h"

// Simple bump allocator + block list (very rudimentary)
// Since this is a unikernel focusing on routing fast path, 
// dynamic allocations will mostly happen at boot time (e.g. mbufs, driver rings).
// For the fast path, we will use pre-allocated pools, so kmalloc doesn't need to be extremely fast.

#define HEAP_SIZE (1024 * 1024 * 16) // 16 MB static heap
static uint8_t heap[HEAP_SIZE] __attribute__((aligned(4096)));
static size_t heap_offset = 0;

void mm_init(void) {
    heap_offset = 0;
}

// A bump allocator (no real free for now, enough for boot-time structs)
void *kmalloc(size_t size) {
    // align to 16 bytes
    size = (size + 15) & ~15;
    
    if (heap_offset + size > HEAP_SIZE) {
        return NULL; // OOM
    }
    
    void *ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

void kfree(void *ptr) {
    // Minimal bump allocator doesn't free.
    // In a real router unikernel, packet buffers are managed by a fixed-size ring or pool.
    (void)ptr;
}
