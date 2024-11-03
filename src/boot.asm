; boot.asm
MBALIGN     equ  1<<0
MEMINFO     equ  1<<1
GRAPHICS    equ  1<<2
FLAGS       equ  MBALIGN | MEMINFO | GRAPHICS
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0        ; unused
    dd 0                    ; graphics mode
    dd 320                  ; width
    dd 200                  ; height
    dd 8                    ; bpp (bits per pixel)

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    
    ; Save multiboot info
    push ebx
    push eax
    
    call kernel_main
    
    cli
.hang:
    hlt
    jmp .hang