// kernel.c

// Type definitions
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t;
#define true 1
#define false 0
#define NULL 0

// VGA text mode constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// Keyboard constants
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// Filesystem constants
#define MAX_FILENAME 32
#define MAX_FILES 64
#define MAX_FILE_SIZE 1024
#define MAX_PATH_LENGTH 256

// VGA text buffer
static uint16_t* const VGA_BUFFER = (uint16_t*)VGA_MEMORY;
static size_t terminal_row = 0;
static size_t terminal_column = 0;

// Filesystem structures
typedef struct {
    char name[MAX_FILENAME];
    uint8_t data[MAX_FILE_SIZE];
    size_t size;
    uint8_t is_directory;
    uint32_t parent_index;
} file_t;

typedef struct {
    file_t files[MAX_FILES];
    size_t file_count;
    char current_path[MAX_PATH_LENGTH];
    uint32_t current_dir_index;
} filesystem_t;

static filesystem_t fs;

// Keyboard buffer
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static size_t buffer_pos = 0;

// Port IO functions
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// String functions
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// Function prototypes
void handle_command(const char* cmd);

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Terminal functions
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
        return;
    }

    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            VGA_BUFFER[terminal_row * VGA_WIDTH + terminal_column] = 0x0700;
        }
        return;
    }

    VGA_BUFFER[terminal_row * VGA_WIDTH + terminal_column] = (uint16_t)c | 0x0700;

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

void terminal_write(const char* str) {
    for (size_t i = 0; str[i]; i++)
        terminal_putchar(str[i]);
}

void terminal_clear() {
    for (size_t i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++)
        VGA_BUFFER[i] = 0x0700;
    terminal_row = 0;
    terminal_column = 0;
}

// Keyboard mapping
static char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static char keyboard_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int shift_pressed = 0;

// Filesystem functions
void fs_init() {
    fs.file_count = 0;
    strcpy(fs.current_path, "/");
    fs.current_dir_index = 0;
    
    // Create root directory
    file_t* root = &fs.files[fs.file_count++];
    strcpy(root->name, "/");
    root->is_directory = 1;
    root->size = 0;
    root->parent_index = 0;
}

file_t* fs_create_file(const char* name, int is_directory) {
    if (fs.file_count >= MAX_FILES) return NULL;
    
    file_t* file = &fs.files[fs.file_count++];
    strcpy(file->name, name);
    file->size = 0;
    file->is_directory = is_directory;
    file->parent_index = fs.current_dir_index;
    return file;
}

file_t* fs_find_file(const char* name) {
    for (size_t i = 0; i < fs.file_count; i++) {
        if (fs.files[i].parent_index == fs.current_dir_index &&
            strcmp(fs.files[i].name, name) == 0) {
            return &fs.files[i];
        }
    }
    return NULL;
}

void fs_change_directory(const char* name) {
    file_t* dir = fs_find_file(name);
    if (dir && dir->is_directory) {
        fs.current_dir_index = dir - fs.files;
        strcpy(fs.current_path, dir->name);
    }
}

// Keyboard handler
void keyboard_handler() {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    
    if (status & 0x01) {
        uint8_t keycode = inb(KEYBOARD_DATA_PORT);
        
        // Handle shift keys
        if (keycode == 0x2A || keycode == 0x36) {
            shift_pressed = 1;
            return;
        }
        if (keycode == 0xAA || keycode == 0xB6) {
            shift_pressed = 0;
            return;
        }

        if (keycode < 128) {
            char c = shift_pressed ? keyboard_map_shift[keycode] : keyboard_map[keycode];
            if (c) {
                if (c == '\b' && buffer_pos > 0) {
                    buffer_pos--;
                    terminal_column--;
                    terminal_putchar(' ');
                    terminal_column--;
                }
                else if (c == '\n') {
                    terminal_putchar('\n');
                    keyboard_buffer[buffer_pos] = 0;
                    handle_command(keyboard_buffer);
                    buffer_pos = 0;
                }
                else if (buffer_pos < KEYBOARD_BUFFER_SIZE - 1) {
                    keyboard_buffer[buffer_pos++] = c;
                    terminal_putchar(c);
                }
            }
        }
    }
}

// Command handling
void handle_command(const char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        terminal_write("Available commands:\n");
        terminal_write("help - Show this help\n");
        terminal_write("clear - Clear screen\n");
        terminal_write("echo <text> - Print text\n");
        terminal_write("ls - List files\n");
        terminal_write("mkdir <name> - Create directory\n");
        terminal_write("touch <name> - Create file\n");
        terminal_write("pwd - Print working directory\n");
        terminal_write("cd <name> - Change directory\n");
    }
    else if (strcmp(cmd, "clear") == 0) {
        terminal_clear();
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        terminal_write(cmd + 5);
        terminal_write("\n");
    }
    else if (strcmp(cmd, "ls") == 0) {
        for (size_t i = 0; i < fs.file_count; i++) {
            if (fs.files[i].parent_index == fs.current_dir_index) {
                if (fs.files[i].is_directory) terminal_write("[DIR] ");
                terminal_write(fs.files[i].name);
                terminal_write("\n");
            }
        }
    }
    else if (strncmp(cmd, "mkdir ", 6) == 0) {
        if (fs_create_file(cmd + 6, 1)) {
            terminal_write("Directory created\n");
        } else {
            terminal_write("Failed to create directory\n");
        }
    }
    else if (strncmp(cmd, "touch ", 6) == 0) {
        if (fs_create_file(cmd + 6, 0)) {
            terminal_write("File created\n");
        } else {
            terminal_write("Failed to create file\n");
        }
    }
    else if (strcmp(cmd, "pwd") == 0) {
        terminal_write(fs.current_path);
        terminal_write("\n");
    }
    else if (strncmp(cmd, "cd ", 3) == 0) {
        fs_change_directory(cmd + 3);
        terminal_write("Directory changed\n");
    }
    else {
        terminal_write("Unknown command: ");
        terminal_write(cmd);
        terminal_write("\n");
    }
}

void kernel_main() {
    // Initialize terminal
    terminal_clear();
    terminal_write("NoirOS v0.1\n");
    terminal_write("Type 'help' for available commands\n");

    // Initialize filesystem
    fs_init();

    // Enable keyboard
    outb(0x64, 0xAE);
    outb(0x60, 0xF4);

    // Main loop
    while (1) {
        keyboard_handler();
    }
}