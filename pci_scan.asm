section .text
global scan_pci

; scan_pci:
; - Arar: Realtek RTL8821CE (Vendor: 0x10EC, Device: 0xC821)
; - Bulursa  eax = 1 → return
; - Bulamazsa eax = 0 → return

scan_pci:
    xor ecx, ecx                ; ecx = bus (0–255)
next_bus:
    xor edx, edx                ; edx = device (0–31)
next_device:
    xor esi, esi                ; esi = function (0–7)
function_loop:

    ; PCI config address hazırla
    mov eax, 0x80000000         ; Enable bit
    mov ebx, ecx
    shl ebx, 16
    or eax, ebx                 ; bus << 16

    mov ebx, edx
    shl ebx, 11
    or eax, ebx                 ; device << 11

    mov ebx, esi
    shl ebx, 8
    or eax, ebx                 ; function << 8

    ; offset = 0 → Vendor & Device ID
    ; >> DX = 0xCF8
    mov dx, 0xCF8
    out dx, eax

    ; << DX = 0xCFC
    mov dx, 0xCFC
    in eax, dx

    cmp eax, 0xFFFFFFFF
    je no_func                 ; cihaz yoksa geç

    cmp ax, 0x8086             ; Vendor ID = Realtek?
    jne no_func

    shr eax, 16
    cmp ax, 0x100E             ; Device ID = RTL8821CE?
    jne no_func

    ; Eşleştiyse → başarılı
    mov eax, 1
    ret

no_func:
    inc esi
    cmp esi, 8
    jl function_loop

    inc edx
    cmp edx, 32
    jl next_device

    inc ecx
    cmp ecx, 256
    jl next_bus

    xor eax, eax               ; bulunamadı
    ret

