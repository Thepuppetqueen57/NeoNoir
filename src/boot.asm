; boot.asm
global start
extern kernel_main

section .multiboot
    align 4
    dd 0x1BADB002            ; Multiboot magic number
    dd 0x00                  ; Flags
    dd -(0x1BADB002 + 0x00)  ; Checksum

section .bss
    align 16
    stack_bottom:
        resb 16384 ; 16 KB
    stack_top:

section .text
start:
    mov esp, stack_top  ; Setup stack
    cli                 ; Disable interrupts
    call kernel_main    ; Call our kernel
    
    ; If kernel returns, just halt
.hang:
    hlt
    jmp .hang