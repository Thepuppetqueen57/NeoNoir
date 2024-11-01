; boot.asm - Boot sector for NoirOS
BITS 16
ORG 0x7C00

jmp short start
nop

; BIOS Parameter Block (BPB)
bpb_oem:                    db 'NoirOS  '
bpb_bytes_per_sector:       dw 512
bpb_sectors_per_cluster:    db 1
bpb_reserved_sectors:       dw 1
bpb_fat_count:              db 2
bpb_root_entries:           dw 224
bpb_total_sectors:          dw 2880
bpb_media_descriptor:       db 0xF0
bpb_sectors_per_fat:        dw 9
bpb_sectors_per_track:      dw 18
bpb_heads:                  dw 2
bpb_hidden_sectors:         dd 0
bpb_large_sectors:          dd 0

start:
    ; Set up segments and stack
    cli
    mov ax, 0x0000
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Switch to VGA graphics mode (320x200, 256 colors)
    mov ax, 0x0013
    int 0x10

    ; Draw the desktop
    call draw_desktop

main_loop:
    ; Check for keyboard input
    mov ah, 0x01
    int 0x16
    jz main_loop

    ; Get key
    mov ah, 0x00
    int 0x16

    ; Check for ESC key
    cmp al, 27
    je shutdown

    jmp main_loop

draw_desktop:
    ; Draw background
    mov ax, 0xA000
    mov es, ax
    xor di, di
    mov cx, 32000  ; 64000/2 (word count)
    mov ax, 0x1717 ; Light blue color
    rep stosw

    ; Draw menu bar
    xor di, di
    mov cx, 160    ; 320/2 (word count)
    mov ax, 0x0F0F ; White color
    rep stosw

    ; Draw menu text
    mov si, menu_text
    xor bx, bx     ; Page 0
    mov dh, 0      ; Row 0
    mov dl, 1      ; Column 1
    call draw_text

    ; Draw windows
    mov si, window1
    call draw_window
    mov si, window2
    call draw_window

    ret

draw_window:
    push si
    lodsw  ; x position
    mov cx, ax
    lodsw  ; y position
    mov dx, ax
    lodsw  ; width
    mov bx, ax
    lodsw  ; height
    mov bp, ax

    ; Draw window frame
    push cx
    push dx
    mov di, 0xA000
    mov ah, 0x0C   ; Draw pixel in graphics mode

    ; Draw top border
    mov cx, bx
    mov dx, 0      ; Start at top
    mov si, di
.top_border:
    mov al, 0x0F   ; White color
    int 0x10       ; Draw pixel
    inc si
    dec cx
    jnz .top_border

    ; Draw bottom border
    mov cx, bx
    mov dx, bp     ; Start at bottom
    add dx, 200    ; Adjust for screen height
    mov si, di
.bottom_border:
    mov al, 0x0F   ; White color
    int 0x10       ; Draw pixel
    inc si
    dec cx
    jnz .bottom_border

    ; Draw left border
    mov cx, bp
    mov dx, 0      ; Start at left
    mov si, di
.left_border:
    mov al, 0x0F   ; White color
    int 0x10       ; Draw pixel
    inc si
    dec cx
    jnz .left_border

    ; Draw right border
    mov cx, bp
    mov dx, bx     ; Start at right
    add dx, 320    ; Adjust for screen width
    mov si, di
.right_border:
    mov al, 0x0F   ; White color
    int 0x10       ; Draw pixel
    inc si
    dec cx
    jnz .right_border

    ; Restore original position
    pop dx
    pop cx

    ; Draw title bar
    mov al, 0x01   ; Blue color
    mov bx, 10     ; Title bar width
    mov cx, bx
    mov dx, 0      ; Start at top
    mov si, di
.title_bar:
    int 0x10       ; Draw pixel
    inc si
    dec cx
    jnz .title_bar

    ; Draw title text
    inc dx
    inc cx
    pop si
    lodsw  ; Skip x and y
    lodsw  ; Skip width and height
    call draw_text

    ret

draw_text:
    push ax
    push bx
    mov ah, 0x0E   ; BIOS teletype function
    mov bl, 0x0F   ; White color on black background
.text_loop:
    lodsb
    test al, al
    jz .text_done
    int 0x10
    inc dl
    jmp .text_loop
.text_done:
    pop bx
    pop ax
    ret

shutdown:
    ; Clear screen and return to text mode
    mov ax, 0x0003
    int 0x10

    ; Print shutdown message
    mov si, shutdown_msg
    call print_string

    ; Wait for key press
    mov ah, 0x00
    int 0x16

    ; Reboot
    int 0x19

print_string:
    mov ah, 0x0E
.print_loop:
    lodsb
    test al, al
    jz .print_done
    int 0x10
    jmp .print_loop
.print_done:
    ret

; Data
menu_text       db 'File  Edit  View  Help', 0
window1         dw 10, 20, 150, 100, 'Window 1', 0
window2         dw 170, 40, 140, 80, 'Window 2', 0
shutdown_msg    db 'Shutting down NoirOS... Press any key to reboot.', 0

times 510-($-$$) db 0
dw 0xAA55