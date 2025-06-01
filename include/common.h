#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

/* I/O helpers – inline asm */
static inline void outb(uint16_t port, uint8_t val)
{ __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port)); }

static inline uint8_t inb(uint16_t port)
{ uint8_t ret; __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }

static inline void io_wait(void) { outb(0x80, 0); }

/* Basit kprint seri log (COM1 = 0x3F8) */
void serial_init(void);
void serial_putchar(char c);
void kprint(const char *s);

#endif

