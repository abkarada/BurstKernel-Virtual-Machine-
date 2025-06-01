#include "common.h"

#define COM1 0x3F8

void serial_init(void)
{
    outb(COM1 + 1, 0x00);    /* interrupts off        */
    outb(COM1 + 3, 0x80);    /* DLAB on               */
    outb(COM1 + 0, 0x03);    /* 38400 baud -> divisor */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);    /* 8n1                  */
    outb(COM1 + 2, 0xC7);    /* FIFO on, 14-byte trig */
}

void serial_putchar(char c)
{
    while (!(inb(COM1 + 5) & 0x20));
    outb(COM1, c);
}

void kprint(const char *s)
{
    while (*s) serial_putchar(*s++);
}

