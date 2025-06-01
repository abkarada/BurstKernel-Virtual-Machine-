/* include/serial.h  –  basit COM1 çıktısı için */

#pragma once
#include <stdarg.h>
#include <stdint.h>

/* Seri portu hazırlar (COM1, 0x3F8) */
void serial_init(void);

/* Tek karakter yazar */
void serial_putchar(char c);

/* Basit string yaz (null-terminated) */
void kprint(const char *s);

/* Kısıtlı printf: %s %c %x %d */
void serial_printf(const char *fmt, ...);
