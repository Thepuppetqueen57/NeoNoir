#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
int cursor_x = 0;
int cursor_y = 0;

// IO functions
void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// VGA entry color
enum vga_color {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHT_GREY,
    DARK_GREY,
    LIGHT_BLUE,
    LIGHT_GREEN,
    LIGHT_CYAN,
    LIGHT_RED,
    LIGHT_MAGENTA,
    LIGHT_BROWN,
    WHITE,
};

uint8_t make_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

void update_cursor() {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

char get_keyboard_char() {
    static const char sc_ascii[] = {
        0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u',
        'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g',
        'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c',
        'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };

    while(1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                if (scancode < sizeof(sc_ascii) && sc_ascii[scancode]) {
                    return sc_ascii[scancode];
                }
            }
        }
        
        // Small delay to prevent overwhelming the CPU
        for(volatile int i = 0; i < 10000; i++) { }
    }
}

uint16_t make_vga_entry(char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

void clear_screen() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            vga_buffer[index] = make_vga_entry(' ', make_color(WHITE, BLACK));
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            const int index = cursor_y * VGA_WIDTH + cursor_x;
            vga_buffer[index] = make_vga_entry(' ', make_color(WHITE, BLACK));
        }
    } else {
        const int index = cursor_y * VGA_WIDTH + cursor_x;
        vga_buffer[index] = make_vga_entry(c, make_color(WHITE, BLACK));
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        // Scroll the screen
        for (int y = 0; y < VGA_HEIGHT - 1; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
        // Clear the last line
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(VGA_HEIGHT-1) * VGA_WIDTH + x] = make_vga_entry(' ', make_color(WHITE, BLACK));
        }
        cursor_y = VGA_HEIGHT - 1;
    }
    update_cursor();
}

void print(const char* str) {
    while (*str) {
        putchar(*str);
        str++;
    }
}

void read_line(char* buffer, int max_length) {
    int i = 0;
    char c;
    while (i < max_length - 1) {
        c = get_keyboard_char();
        if (c == '\n') {
            buffer[i] = '\0';
            putchar('\n');
            return;
        } else if (c == '\b' && i > 0) {
            i--;
            putchar('\b');
        } else if (c >= 32 && c <= 126) {
            buffer[i] = c;
            putchar(c);
            i++;
        }
    }
    buffer[i] = '\0';
}

void execute_command(const char* command) {
    if (strcmp(command, "clear") == 0) {
        clear_screen();
    } else if (strcmp(command, "help") == 0) {
        print("Available commands:\n");
        print("  clear - Clear the screen\n");
        print("  help - Show this help message\n");
    } else {
        print("Unknown command: ");
        print(command);
        print("\n");
    }
}

void shell() {
    char command[256];
    while (1) {
        print("NoirOS> ");
        read_line(command, sizeof(command));
        execute_command(command);
    }
}

int kernel_main() {
    clear_screen();
    print("Welcome to NoirOS!\n");
    print("Type 'help' for a list of commands.\n\n");
    shell();
    return 0;
}