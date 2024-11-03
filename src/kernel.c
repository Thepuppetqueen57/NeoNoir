typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t*)0xB8000)
#define WHITE_ON_BLACK 0x0F

unsigned int cursor_position = 0;

void vga_put_char(char c, unsigned char color, unsigned int x, unsigned int y) {
    unsigned int index = (y * VGA_WIDTH + x);
    VGA_MEMORY[index] = ((uint16_t)color << 8) | (uint16_t)c;
}

void vga_print(const char* str) {
    for (unsigned int i = 0; str[i] != '\0'; ++i) {
        if (str[i] == '\n') {
            cursor_position += VGA_WIDTH - (cursor_position % VGA_WIDTH);
        } else {
            vga_put_char(str[i], WHITE_ON_BLACK, cursor_position % VGA_WIDTH, cursor_position / VGA_WIDTH);
            cursor_position++;
        }
    }
}

void init_vga() {
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = (WHITE_ON_BLACK << 8) | ' ';
    }
    cursor_position = 0;
}

void kernel_main() {
    // Write directly to video memory as a test
    volatile unsigned char *video = (volatile unsigned char*)0xB8000;
    video[0] = 'X';
    video[1] = 0x0F;  // White on black

    init_vga();
    vga_print("Initialized NoirOS Kernel.\n");
    vga_print("Welcome to NoirOS!\n");

    while(1) {
        // Kernel main loop
        asm volatile("hlt");
    }
}