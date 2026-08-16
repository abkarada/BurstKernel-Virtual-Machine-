.code64
.global isr1
.extern keyboard_handler

isr1:
    push %rax
    push %rcx
    push %rdx
    push %r8
    push %r9
    push %r10
    push %r11
    
    call keyboard_handler
    
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdx
    pop %rcx
    pop %rax
    iretq

.global isr11
.extern e1000_handle_interrupt

isr11:
    push %rax
    push %rcx
    push %rdx
    push %r8
    push %r9
    push %r10
    push %r11
    
    call e1000_handle_interrupt
    
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdx
    pop %rcx
    pop %rax
    iretq
