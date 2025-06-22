.section .multiboot
.align 4
.long 0x1BADB002
.long 0x0
.long -(0x1BADB002 + 0x0)

.section .text
.global _start 
.type _start, @function
.extern _stack_top;
.extern kmain

_start:
	mov $(_stack_top), %esp
	call kmain
	cli
.hang:
	hlt
	jmp .hang
