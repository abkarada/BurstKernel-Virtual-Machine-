.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set MAGIC,    0x1BADB002
.set FLAGS,    ALIGN | MEMINFO
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot, "a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 4096
p4_table:
    .skip 4096
p3_table:
    .skip 4096
p2_table:
    .skip 16384

.align 16
stack_bottom:
    .skip 16384 # 16 KiB
stack_top:

.section .rodata
.align 8
gdt64:
    .quad 0 # zero entry
.equ gdt64_code_segment, . - gdt64
    # code segment: executable, read, 64-bit
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<53)
.equ gdt64_data_segment, . - gdt64
    # data segment: read/write, present
    .quad (1<<41) | (1<<44) | (1<<47)
gdt64_pointer:
    .word . - gdt64 - 1
    .long gdt64
    .long 0

.section .text
.code32
.global _start
.type _start, @function
_start:
    # Stack setup
    mov $stack_top, %esp

    # Setup Page Tables (Identity Map 4GB)
    
    # 1. Link p4 to p3
    mov $p3_table, %eax
    or $0x3, %eax      # Present, Write
    mov %eax, p4_table

    # 2. Link p3 to p2 (4 entries for 4GB)
    mov $p2_table, %eax
    or $0x3, %eax
    mov %eax, p3_table
    add $4096, %eax
    mov %eax, p3_table + 8
    add $4096, %eax
    mov %eax, p3_table + 16
    add $4096, %eax
    mov %eax, p3_table + 24

    # 3. Fill 2048 entries in p2_table (2048 * 2MB = 4GB)
    mov $0x0, %ebx
    mov $0x83, %eax # Present, Write, Huge (2MB)
1:
    mov %eax, p2_table(,%ebx,8)
    add $0x200000, %eax
    inc %ebx
    cmp $2048, %ebx
    jne 1b

    # Load CR3
    mov $p4_table, %eax
    mov %eax, %cr3

    # Enable PAE in CR4
    mov %cr4, %eax
    or $1<<5, %eax
    mov %eax, %cr4

    # Set LME in EFER MSR
    mov $0xC0000080, %ecx
    rdmsr
    or $1<<8, %eax
    wrmsr

    # Enable Paging
    mov %cr0, %eax
    or $1<<31, %eax
    mov %eax, %cr0

    # Load 64-bit GDT
    lgdt gdt64_pointer

    # Far jump into 64-bit mode
    ljmp $gdt64_code_segment, $start64

.code64
.extern kmain
start64:
    # Update data segment registers
    mov $gdt64_data_segment, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    call kmain
    cli
1:  hlt
    jmp 1b
