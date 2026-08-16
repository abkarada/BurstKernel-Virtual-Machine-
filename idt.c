#include <stdint.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

extern void isr1(void);
extern void isr11(void);
extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = (base & 0xFFFF);
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].flags = flags;
    idt[num].zero = 0;
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint64_t)&idt;

    // Remap PIC (Master: 0x20-0x27, Slave: 0x28-0x2F)
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    
    // Mask all initially
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    // Set ISR1 (IRQ1 - Keyboard) at interrupt 0x21 (0x20 + 1)
    idt_set_gate(33, (uint64_t)isr1, 0x08, 0x8E); // 0x08 is our 64-bit code segment from boot.s

    // Set ISR11 (IRQ11 - E1000) at interrupt 0x2B (0x20 + 11 = 43)
    idt_set_gate(43, (uint64_t)isr11, 0x08, 0x8E);

    // Unmask IRQ11 (Slave PIC IRQ3 -> 11 - 8 = 3)
    // We also need to unmask IRQ2 on Master PIC so Slave PIC can interrupt
    outb(0x21, inb(0x21) & ~(1 << 2)); // Unmask cascade
    outb(0xA1, inb(0xA1) & ~(1 << 3)); // Unmask IRQ11

    // Load IDT
    __asm__ volatile("lidt %0" : : "m" (idtp));
}
