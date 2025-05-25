.set MULTIBOOT_HEADER_MAGIC, 0x1BADB002
.set MULTIBOOT_HEADER_FLAGS, 0x0
.set MULTIBOOT_HEADER_CHECKSUM, -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.section .multiboot
.align 4
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long MULTIBOOT_HEADER_CHECKSUM

.section .text
.global _start
_start:
    cli
    call kmain
hang:
    hlt
    jmp hang
