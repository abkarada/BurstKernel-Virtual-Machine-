#ifndef IO_H
#define IO_H

#include <stdint.h>

/// Write one byte to an I/O port
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(val), "Nd"(port)
    );
}

/// Read one byte from an I/O port
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile (
        "inb %1, %0"
        : "=a"(ret)
        : "Nd"(port)
    );
    return ret;
}

/// I/O delay (yavaşlatmak için boş işlem)
static inline void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif // IO_H
