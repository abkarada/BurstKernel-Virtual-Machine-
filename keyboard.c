#include <stdint.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);
extern void puts(const char *s);
extern void puts_serial(const char *s);

// A very basic US layout scancode map for pressed keys
static const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
  '9', '0', '-', '=', '\b', /* Backspace */
  '\t',     /* Tab */
  'q', 'w', 'e', 'r',   /* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
    0,      /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
 '\'', '`',   0,        /* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',      /* 49 */
  'm', ',', '.', '/',   0,        /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0, /* All other keys are undefined */
};

#define SHELL_BUF_SIZE 256
static char shell_buf[SHELL_BUF_SIZE];
static int shell_idx = 0;

#include "cli.h"

void shell_execute() {
    shell_buf[shell_idx] = '\0';
    if (shell_idx > 0) {
        puts("\n");
        cli_execute(shell_buf);
    } else {
        puts("\n");
    }
    shell_idx = 0;
    
    // Oh My Zsh style colorful prompt
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    puts("➜  ");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    puts("nkernel ");
    set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
    puts("✗ ");
    set_color(VGA_WHITE, VGA_BLACK); // Reset
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    // Only handle key presses (top bit clear)
    if (!(scancode & 0x80)) {
        char c = kbd_us[scancode];
        if (c != 0) {
            if (c == '\n') {
                shell_execute();
            } else if (c == '\b') {
                if (shell_idx > 0) {
                    shell_idx--;
                    // For backspace, we should really update the VGA cursor, 
                    // but for now just print a backspace character to serial and 
                    // a space to VGA if we had a full driver. 
                    // Minimal implementation:
                    puts("\b \b");
                }
            } else {
                if (shell_idx < SHELL_BUF_SIZE - 1) {
                    shell_buf[shell_idx++] = c;
                    char str[2] = {c, '\0'};
                    puts(str);
                    puts_serial(str);
                }
            }
        }
    }
    
    // Send EOI (End of Interrupt) to PIC
    outb(0x20, 0x20);
}

void keyboard_init(void) {
    // Oh My Zsh style colorful prompt
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    puts("➜  ");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    puts("nkernel ");
    set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
    puts("✗ ");
    set_color(VGA_WHITE, VGA_BLACK); // Reset
    // Enable IRQ1 (Keyboard) on Master PIC
    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~(1 << 1));
}
