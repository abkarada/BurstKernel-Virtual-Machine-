.section .multiboot
.align 4
.long 0x1BADB002            # magic
.long 0x00000000            # flags
.long 0xE4524FFE            # checksum = -(magic + flags)

.section .text
.global _start

_start:
    call kmain
hang:
    jmp hang
