typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

void kernel_main();

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

#define HEAP_START 0x100000
#define HEAP_SIZE 0x10000
unsigned int heap_end = HEAP_START;

void* malloc(unsigned int size) {
    if (heap_end + size > HEAP_START + HEAP_SIZE) return 0;
    void* ptr = (void*)heap_end;
    heap_end += size;
    return ptr;
}

enum SyscallNumbers { SYS_PRINT, SYS_EXIT };

void syscall(int number, void* arg) {
    switch (number) {
        case SYS_PRINT:
            vga_print((const char*)arg);
            break;
        case SYS_EXIT:
            while (1) {}
    }
}

char keyboard_map[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, 0,
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static inline void outb(unsigned short port, unsigned char value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void keyboard_handler() {
    unsigned char scancode = inb(0x60);
    if (scancode < 128 && keyboard_map[scancode] != 0) {
        char key = keyboard_map[scancode];
        vga_print(&key);
    }
}

void mouse_handler() {
    unsigned char status = inb(0x64);
    if (status & 1) {
        unsigned char mouse_data = inb(0x60);
        if (mouse_data & 0x08) {
            vga_print("Mouse moved\n");
        }
    }
}

void init_devices() {
    outb(0x60, 0xAE);
    outb(0x60, 0xF4);
}

void kernel_main() {
    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = (WHITE_ON_BLACK << 8) | ' ';
    }

    vga_print("Initialized NoirOS Kernel. Entering userspace..\n");

    init_devices();

    syscall(SYS_EXIT, 0);
}