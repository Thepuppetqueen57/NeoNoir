typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int size_t;

int strlen(const char *str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

#define NULL 0

#define MEMORY_POOL_SIZE (1024 * 1024)  // 1 MB memory pool

typedef struct block_meta {
    size_t size;
    struct block_meta *next;
    int free;
    int magic;  // For debugging
} block_meta;

#define META_SIZE sizeof(block_meta)

void *global_base = NULL;

// Function prototypes
void *malloc(size_t size);
void free(void *ptr);
void *find_free_block(block_meta **last, size_t size);
block_meta *request_space(block_meta *last, size_t size);

// Initialize the memory pool
static char memory_pool[MEMORY_POOL_SIZE];
static int memory_initialized = 0;

void init_memory() {
    if (!memory_initialized) {
        global_base = memory_pool;
        memory_initialized = 1;
    }
}

void *malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    if (!memory_initialized) {
        init_memory();
    }

    block_meta *block;

    // Make sure we allocate enough space for the metadata
    size_t aligned_size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
    size_t total_size = aligned_size + META_SIZE;

    if (global_base) {  // We have existing allocations
        block_meta *last = global_base;
        block = find_free_block(&last, total_size);
        if (!block) {  // Failed to find free block
            block = request_space(last, total_size);
            if (!block) {
                return NULL;
            }
        } else {  // Found free block
            block->free = 0;
            block->magic = 0x12345678;
        }
    } else {  // First allocation
        block = request_space(NULL, total_size);
        if (!block) {
            return NULL;
        }
        global_base = block;
    }

    return (block + 1);  // Return pointer to region after block metadata
}

void *find_free_block(block_meta **last, size_t size) {
    block_meta *current = global_base;
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

block_meta *request_space(block_meta *last, size_t size) {
    block_meta *block;
    block = (block_meta *)((char *)global_base + MEMORY_POOL_SIZE - size);

    if ((void *)block < (void *)global_base) {
        return NULL;  // No more memory
    }

    if (last) {
        last->next = block;
    }
    block->size = size;
    block->next = NULL;
    block->free = 0;
    block->magic = 0x12345678;
    return block;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }

    // Get the block metadata
    block_meta *block_ptr = (block_meta *)ptr - 1;

    // Basic sanity check
    if (block_ptr->magic != 0x12345678) {
        return;
    }

    // Mark the block as free
    block_ptr->free = 1;
    block_ptr->magic = 0x87654321;

    // Optional: Coalesce free blocks
    block_meta *current = global_base;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += current->next->size + META_SIZE;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

#define VGA_WIDTH 80
#define VGA_HEIGHT 26
#define VGA_MEMORY 0xB8000

void update_cursor(void);
uint16_t make_vga_entry(char c, uint8_t color);

int cursor_x, cursor_y;
uint16_t *vga_buffer = (uint16_t *)0xB8000;

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

#define KEYBOARD_PORT 0x60
#define KEYBOARD_STATUS 0x64
#define SHUTDOWN_PORT1 0xB004
#define SHUTDOWN_PORT2 0x2000
#define SHUTDOWN_PORT3 0x604

#define MAX_FILENAME 32
#define MAX_FILES 64
#define BLOCK_SIZE 512
#define DATA_BLOCKS 1024

uint8_t make_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
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
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
                make_vga_entry(' ', make_color(WHITE, BLACK));
        }
        cursor_y = VGA_HEIGHT - 1;
    }
    update_cursor();
}

void print(const char *str) {
    while (*str) {
        putchar(*str);
        str++;
    }
}

void print_colored(const char *str, uint8_t color) {
    int current_x = cursor_x;
    int current_y = cursor_y;

    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            const int index = cursor_y * VGA_WIDTH + cursor_x;
            vga_buffer[index] = make_vga_entry(*str, color);
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
                vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_vga_entry(' ', color);
            }
            cursor_y = VGA_HEIGHT - 1;
        }
        str++;
    }
    update_cursor();
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strncpy(char *dest, const char *src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

void memset(void *ptr, int value, uint32_t num) {
    uint8_t *p = (uint8_t *)ptr;
    while (num--) {
        *p++ = (uint8_t)value;
    }
}

void *memcpy(void *dest, const void *src, uint32_t num) {
    // Cast the destination and source pointers to byte pointers
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    // Copy bytes from source to destination
    while (num--) {
        *d++ = *s++;  // Copy byte by byte
    }

    return dest;  // Return the destination pointer
}

typedef struct FileEntry {
    char filename[MAX_FILENAME];
    uint32_t size;
    uint32_t start_block;
    int is_directory;
    struct Directory *dir_ptr;  // Pointer to Directory if this is a directory
} FileEntry;

typedef struct Directory {
    char name[MAX_FILENAME];
    uint32_t start_block;
    uint32_t num_files;
    FileEntry files[MAX_FILES];
    struct Directory *parent;  // optional, if needed
} Directory;

typedef struct {
    Directory root;
    Directory *current_dir;
    uint32_t num_files;
    FileEntry files[MAX_FILES];
} FileSystem;

FileSystem fs;
uint8_t disk[BLOCK_SIZE * (DATA_BLOCKS + 1)];

// Filesystem

void init_fs() {
    memset(&fs, 0, sizeof(FileSystem));
    memset(disk, 0, sizeof(disk));

    // Initialize root directory
    strncpy(fs.root.name, "/", MAX_FILENAME);
    fs.root.start_block = 1;  // Start after the metadata
    fs.root.num_files = 0;
    fs.current_dir = &fs.root;
}

int create_file(const char *filename, const char *content) {
    if (fs.current_dir->num_files >= MAX_FILES) {
        print("Error: Directory is full\n");
        return -1;  // Directory is full
    }

    // Check if a file with this name already exists
    for (uint32_t i = 0; i < fs.current_dir->num_files; i++) {
        if (strcmp(fs.current_dir->files[i].filename, filename) == 0) {
            print("Error: File already exists with this name\n");
            return -1;  // File already exists
        }
    }

    // Calculate the size of the content
    size_t content_size = strlen(content) + 1;  // +1 for null terminator

    // Allocate memory for the file content
    char *file_content = (char *)malloc(content_size);
    if (file_content == NULL) {
        print("Error: Failed to allocate memory for file content\n");
        return -1;  // Memory allocation failed
    }

    // Copy the content to the allocated memory
    strncpy(file_content, content, content_size);

    // Create a new FileEntry for the file
    FileEntry *new_entry = &fs.current_dir->files[fs.current_dir->num_files];
    strncpy(new_entry->filename, filename, MAX_FILENAME);
    new_entry->size = content_size;
    new_entry->start_block = (uint32_t)file_content;  // Use the pointer as the "block" address
    new_entry->is_directory = 0;

    fs.current_dir->num_files++;

    return 0;
}

int read_file(const char *filename, void *buffer, uint32_t size) {
    int file_index = -1;
    for (int i = 0; i < fs.num_files; i++) {
        if (strcmp(fs.files[i].filename, filename) == 0) {
            file_index = i;
            break;
        }
    }

    if (file_index == -1) {
        return -1;  // File not found
    }

    FileEntry *file = &fs.files[file_index];
    if (size > file->size) {
        size = file->size;
    }

    memcpy(buffer, &disk[file->start_block * BLOCK_SIZE], size);
    return size;
}

void save_fs() {
    memcpy(disk, &fs, sizeof(FileSystem));
}

void load_fs() {
    memcpy(&fs, disk, sizeof(FileSystem));
}

// End of Filesystem

// FS Commands

int touch(const char *filename) {
    return create_file(filename, NULL);  // Pass NULL for empty file
}

void cat(const char *filename) {
    // Find the file
    FileEntry *file = NULL;
    for (int i = 0; i < fs.current_dir->num_files; i++) {
        if (strcmp(fs.current_dir->files[i].filename, filename) == 0) {
            file = &fs.current_dir->files[i];
            break;
        }
    }

    if (file == NULL) {
        print("Error: File not found\n");
        return;
    }

    // Check if it's a directory
    if (file->is_directory) {
        print("Error: Cannot cat a directory\n");
        return;
    }

    // Print the contents of the file
    char *content = (char *)file->start_block;
    print(content);
    print("\n");
}

int mkdir(const char *dirname) {
    if (fs.current_dir->num_files >= MAX_FILES) {
        print("Error: Directory is full\n");
        return -1;  // Directory is full
    }

    // Check if a file or directory with this name already exists
    for (uint32_t i = 0; i < fs.current_dir->num_files; i++) {
        if (strcmp(fs.current_dir->files[i].filename, dirname) == 0) {
            print("Error: File or directory already exists with this name\n");
            return -1;  // File or directory already exists
        }
    }

    // Create a new FileEntry for the directory
    FileEntry *new_entry = &fs.current_dir->files[fs.current_dir->num_files];
    strncpy(new_entry->filename, dirname, MAX_FILENAME);
    new_entry->size = 0;  // Directories don't have a size in this simple implementation
    new_entry->start_block =
        0;  // You'll need to implement block allocation if you add that feature
    new_entry->is_directory = 1;

    // Create a new Directory structure
    Directory *new_dir = (Directory *)malloc(sizeof(Directory));
    if (new_dir == NULL) {
        print("Error: Failed to allocate memory for new directory\n");
        return -1;  // Memory allocation failed
    }

    strncpy(new_dir->name, dirname, MAX_FILENAME);
    new_dir->start_block = 0;  // You'll need to implement block allocation if you add that feature
    new_dir->num_files = 0;
    new_dir->parent = fs.current_dir;

    new_entry->dir_ptr = new_dir;

    fs.current_dir->num_files++;

    return 0;
}

void ls() {
    for (int i = 0; i < fs.current_dir->num_files; i++) {
        print(fs.current_dir->files[i].filename);
        print("\n");
    }
}

int cd(const char *dirname) {
    if (strcmp(dirname, "..") == 0) {
        if (fs.current_dir->parent != NULL) {
            fs.current_dir = fs.current_dir->parent;
            return 0;
        }
        return -1;  // Already at root
    }

    for (int i = 0; i < fs.current_dir->num_files; i++) {
        if (strcmp(fs.current_dir->files[i].filename, dirname) == 0) {
            if (fs.current_dir->files[i].is_directory) {
                fs.current_dir = fs.current_dir->files[i].dir_ptr;
                return 0;
            } else {
                return -1;  // Not a directory
            }
        }
    }

    return -1;  // Directory not found
}

// End of FS Commands

// Function prototypes

void print_colored(const char *str, unsigned char color);

void update_cursor(void);

uint16_t make_vga_entry(char c, unsigned char color);

void play_sound(unsigned int frequency);

void stop_sound(void);

void sleep(unsigned int milliseconds);

void play_silly_tune(void);

// IO functions
void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void update_cursor() {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

#define LSHIFT 0x2A
#define RSHIFT 0x36
#define CAPS_LOCK 0x3A

char get_keyboard_char() {
    static const char sc_ascii[] = {0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
                                    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
                                    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
                                    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
                                    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' '};

    static const char sc_ascii_shift[] = {
        0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
        'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',  'Z',
        'X',  'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' '};

    static int shift = 0;
    static int caps_lock = 0;

    while (1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);

            if (scancode == LSHIFT || scancode == RSHIFT) {
                shift = 1;
                continue;
            } else if (scancode == (LSHIFT | 0x80) || scancode == (RSHIFT | 0x80)) {
                shift = 0;
                continue;
            } else if (scancode == CAPS_LOCK) {
                caps_lock = !caps_lock;
                continue;
            }

            if (!(scancode & 0x80)) {
                if (scancode < sizeof(sc_ascii)) {
                    char c;
                    if (shift) {
                        c = sc_ascii_shift[scancode];
                    } else {
                        c = sc_ascii[scancode];
                    }

                    if (caps_lock && c >= 'a' && c <= 'z') {
                        c -= 32;  // Convert to uppercase
                    } else if (caps_lock && c >= 'A' && c <= 'Z') {
                        c += 32;  // Convert to lowercase
                    }

                    if (c) {
                        return c;
                    }
                }
            }
        }

        // Small delay to prevent overwhelming the CPU
        for (volatile int i = 0; i < 10000; i++) {
        }
    }
}

uint16_t make_vga_entry(char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
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

void cpuinfo();
void time();
void calc(const char *expression);

void read_line(char *buffer, int max_length) {
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

#ifndef UINT_TYPES
#define UINT_TYPES
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
#endif

// ACPI-related definitions
#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_FADT_SIGNATURE "FACP"
#define ACPI_DSDT_SIGNATURE "DSDT"

// PCI-related definitions
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// Keyboard controller
#define KBC_DATA_PORT 0x60
#define KBC_STATUS_PORT 0x64

// CMOS
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

// Function prototypes
void shutdown();
void *find_acpi_table(const char *signature);
void acpi_poweroff();
void apm_poweroff();
void pci_reset();
void ps2_reset();
void cmos_reset();
void triple_fault();
void outl(uint16_t port, uint32_t val);

void outl(uint16_t port, uint32_t val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

void io_wait() {
    outb(0x80, 0);
}

#define NULL 0

// Type definitions (since we can't use stdint.h)
#ifndef UINT_TYPES
#define UINT_TYPES
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif

// ACPI-related definitions
#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_FADT_SIGNATURE "FACP"
#define ACPI_DSDT_SIGNATURE "DSDT"

// PCI-related definitions
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// Keyboard controller
#define KBC_DATA_PORT 0x60
#define KBC_SATUS_PORT 0x64

// CMOS
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

// Custom memcmp implementation
int memcmp(const void *s1, const void *s2, int n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

// Main shutdown function
void shutdown() {
    print("Initiating NoirOS advanced shutdown sequence...\n");

    // 1. Try ACPI shutdown
    print("Attempting ACPI shutdown...\n");
    acpi_poweroff();
    io_wait();

    // 2. Try APM shutdown
    print("Attempting APM shutdown...\n");
    outw(0xB004, 0x0 | (2 << 10));
    io_wait();

    // 3. Try PCI reset
    print("Attempting PCI reset...\n");
    outl(PCI_CONFIG_ADDRESS, 0x8000F840);
    outb(PCI_CONFIG_DATA, 0x0E);
    io_wait();

    // 4. Try PS/2 keyboard controller reset
    print("Attempting PS/2 keyboard controller reset...\n");
    while (inb(KBC_STATUS_PORT) & 2);
    outb(KBC_STATUS_PORT, 0xFE);
    io_wait();

    // 5. Try CMOS reset
    print("Attempting CMOS reset...\n");
    outb(CMOS_ADDRESS, 0xF);
    outb(CMOS_DATA, 0x0A);
    io_wait();

    // 6. Try keyboard controller reset
    print("Attempting keyboard controller reset...\n");
    uint8_t good = 0x02;
    while (good & 0x02) good = inb(KEYBOARD_STATUS);
    outb(KEYBOARD_STATUS, 0xFE);
    io_wait();

    // 7. Try to exit QEMU (won't affect real hardware)
    print("Attempting QEMU exit...\n");
    outw(0x604, 0x2000);

    // 8. Last resort: Triple fault
    print("All shutdown attempts failed. Initiating triple fault...\n");
    asm volatile(
        "movl $0, %eax\n"
        "movl %eax, (%eax)\n");

    // If we're still here, nothing worked
    print("System is still running. It is now safe to power off your computer.\n");

    // Disable interrupts and halt
    asm volatile("cli");
    while (1) {
        asm volatile("hlt");
    }
}

// ACPI power off helper function
void acpi_poweroff() {
    // Search for RSDP in BIOS memory
    char *addr;
    for (addr = (char *)0x000E0000; addr < (char *)0x00100000; addr += 16) {
        if (memcmp(addr, ACPI_RSDP_SIGNATURE, 8) == 0) {
            // Found RSDP, now try to shut down
            uint32_t pm1a_cnt = 0x1000;     // Default port if we can't find real one
            uint16_t slp_typa = (1 << 13);  // Default sleep type

            outw(pm1a_cnt, slp_typa | (1 << 13));
            io_wait();
            return;
        }
    }
}

// ACPI power off
void *find_acpi_table(const char *signature) {
    // Search for RSDP in BIOS memory
    for (char *addr = (char *)0x000E0000; addr < (char *)0x00100000; addr += 16) {
        if (memcmp(addr, ACPI_RSDP_SIGNATURE, 8) == 0) {
            // Found RSDP, now find RSDT
            uint32_t *rsdt = (uint32_t *)(*(uint32_t *)(addr + 16));
            int entries = (*(uint32_t *)(addr + 4) - 36) / 4;

            // Search RSDT for the requested table
            for (int i = 0; i < entries; i++) {
                void *table = (void *)rsdt[i + 9];
                if (memcmp(table, signature, 4) == 0) {
                    return table;
                }
            }
        }
    }
    return NULL;
}

// APM power off
void apm_poweroff() {
    outw(0xB004, 0x0 | (2 << 10));
    io_wait();
}

// PCI reset
void pci_reset() {
    outl(PCI_CONFIG_ADDRESS, 0x8000F840);
    outb(PCI_CONFIG_DATA, 0x0E);
    io_wait();
}

// PS/2 keyboard controller reset
void ps2_reset() {
    while (inb(KBC_STATUS_PORT) & 2);
    outb(KBC_STATUS_PORT, 0xFE);
    io_wait();
}

// CMOS reset
void cmos_reset() {
    outb(CMOS_ADDRESS, 0xF);
    outb(CMOS_DATA, 0x0A);
    io_wait();
}

// Triple fault (last resort)
void triple_fault() {
    asm volatile(
        "movl $0, %eax\n"
        "movl %eax, (%eax)\n");
}

// Add these type definitions if not already present
typedef unsigned int uint32_t;
typedef int int32_t;

// String parsing function for the calculator
int parse_number(const char **str) {
    int num = 0;
    int sign = 1;

    // Skip whitespace
    while (**str == ' ') (*str)++;

    // Handle negative numbers
    if (**str == '-') {
        sign = -1;
        (*str)++;
    }

    while (**str >= '0' && **str <= '9') {
        num = num * 10 + (**str - '0');
        (*str)++;
    }

    return num * sign;
}

char parse_operator(const char **str) {
    // Skip whitespace
    while (**str == ' ') (*str)++;

    char op = **str;
    if (op) (*str)++;

    return op;
}

// Modified calculator function
void calc(const char *expression) {
    const char *ptr = expression;
    int a, b, result;
    char op;

    // Parse first number
    a = parse_number(&ptr);

    // Parse operator
    op = parse_operator(&ptr);

    // Parse second number
    b = parse_number(&ptr);

    switch (op) {
        case '+':
            result = a + b;
            break;
        case '-':
            result = a - b;
            break;
        case '*':
            result = a * b;
            break;
        case '/':
            if (b == 0) {
                print("Error: Division by zero\n");
                return;
            }
            result = a / b;
            break;
        default:
            print("Invalid operator. Supported operators: +, -, *, /\n");
            return;
    }

    // Print result using our own number to string conversion
    char buffer[20];
    int i = 0;
    int is_negative = 0;
    int temp;

    // Convert first number
    if (a < 0) {
        putchar('-');
        a = -a;
    }
    temp = a;
    do {
        buffer[i++] = (temp % 10) + '0';
        temp /= 10;
    } while (temp > 0);
    while (i > 0) putchar(buffer[--i]);

    // Print operator
    putchar(' ');
    putchar(op);
    putchar(' ');

    // Convert second number
    if (b < 0) {
        putchar('-');
        b = -b;
    }
    temp = b;
    do {
        buffer[i++] = (temp % 10) + '0';
        temp /= 10;
    } while (temp > 0);
    while (i > 0) putchar(buffer[--i]);

    print(" = ");

    // Convert result
    if (result < 0) {
        putchar('-');
        result = -result;
    }
    temp = result;
    do {
        buffer[i++] = (temp % 10) + '0';
        temp /= 10;
    } while (temp > 0);
    while (i > 0) putchar(buffer[--i]);

    print("\n");
}

void play_sound(unsigned int frequency);
void stop_sound(void);
void sleep(unsigned int milliseconds);
void play_silly_tune(void);

void play_sound(unsigned int frequency) {
    unsigned int div;
    unsigned char tmp;

    // Set the PIT to the desired frequency
    div = 1193180 / frequency;
    outb(0x43, 0xb6);
    outb(0x42, (unsigned char)(div));
    outb(0x42, (unsigned char)(div >> 8));

    // And play the sound using the PC speaker
    tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

void stop_sound(void) {
    unsigned char tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

void sleep(unsigned int milliseconds) {
    for (unsigned int i = 0; i < milliseconds * 10000; i++) {
        __asm__ __volatile__("nop");
    }
}

void play_silly_tune(void) {
    // Define some basic frequencies
    const unsigned int C4 = 262;
    const unsigned int D4 = 294;
    const unsigned int E4 = 330;
    const unsigned int F4 = 349;
    const unsigned int G4 = 392;
    const unsigned int A4 = 440;
    const unsigned int B4 = 494;
    const unsigned int C5 = 523;

    print_colored("Playing a silly tune...\n", make_color(LIGHT_CYAN, BLACK));

    const unsigned int notes[] = {E4, D4, C4, D4, E4, E4, E4, D4, D4, D4, E4, G4, G4, E4, D4, C4,
                                  D4, E4, E4, E4, E4, D4, D4, E4, D4, C4,

                                  // New section
                                  C4, C4, D4, E4, E4, D4, C4, C4, D4, E4, E4, D4, C4, C4, D4, E4,

                                  // Another variation
                                  G4, G4, A4, B4, B4, A4, G4, G4, A4, B4, B4, A4, G4, G4, A4, B4,

                                  // Final section
                                  E4, D4, C4, D4, E4, E4, E4, D4, D4, D4, E4, G4, G4, E4, D4, C4,
                                  D4, E4, E4, E4, E4, D4, D4, E4, D4, C4};

    const unsigned int durations[] = {
        200, 200, 200, 200, 200, 200, 400, 200, 200, 400, 200, 200, 400, 200, 200, 200, 200, 200,
        200, 400, 200, 200, 200, 200, 200, 400,

        // New section durations
        200, 200, 200, 200, 200, 200, 400, 200, 200, 200, 200, 200, 200, 400, 200,

        // Another variation durations
        200, 200, 200, 200, 200, 200, 400, 200, 200, 200, 200, 200, 200, 400, 200,

        // Final section durations
        200, 200, 200, 200, 200, 200, 400, 200, 200, 400, 200, 200, 400, 200, 200, 200, 200, 200,
        200, 400, 200, 200, 200, 200, 200, 400};

    for (int i = 0; i < sizeof(notes) / sizeof(notes[0]); i++) {
        play_sound(notes[i]);
        sleep(durations[i]);
        stop_sound();
        sleep(50);  // Small pause between notes
    }

    print_colored("Song finished!\n", make_color(LIGHT_GREEN, BLACK));
}

// Modified cpuinfo function
// Function to emulate CPUID instruction
static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx,
                         uint32_t *ecx, uint32_t *edx) {
    __asm__ __volatile__("cpuid"
                         : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                         : "a"(leaf), "c"(subleaf));
}

// Function prototypes for the adventure game
void adventure_north(void);
void adventure_south(void);
void adventure_east(void);
void adventure_west(void);
void textadventure(void);

void textadventure() {
    clear_screen();
    print_colored("Welcome to the Text Adventure Game!\n", make_color(LIGHT_CYAN, BLACK));
    print_colored("You find yourself in a dark forest. You can go:\n",
                  make_color(LIGHT_GREEN, BLACK));
    print_colored("1. North\n", make_color(WHITE, BLACK));
    print_colored("2. South\n", make_color(WHITE, BLACK));
    print_colored("3. East\n", make_color(WHITE, BLACK));
    print_colored("4. West\n", make_color(WHITE, BLACK));
    print_colored("Type your choice (1-4): ", make_color(LIGHT_GREY, BLACK));

    char choice[2];
    read_line(choice, sizeof(choice));

    if (strcmp(choice, "1") == 0) {
        adventure_north();
    } else if (strcmp(choice, "2") == 0) {
        adventure_south();
    } else if (strcmp(choice, "3") == 0) {
        adventure_east();
    } else if (strcmp(choice, "4") == 0) {
        adventure_west();
    } else {
        print_colored("Invalid choice. Game over.\n", make_color(RED, BLACK));
    }
}

void adventure_north() {
    clear_screen();
    print_colored("You go north and find a mysterious treasure chest!\n",
                  make_color(LIGHT_BROWN, BLACK));
    print_colored("The chest is covered in ancient runes and seems to glow...\n",
                  make_color(LIGHT_BLUE, BLACK));
    print_colored("Do you want to open it? (y/n): ", make_color(WHITE, BLACK));

    char choice[2];
    read_line(choice, sizeof(choice));

    if (strcmp(choice, "y") == 0) {
        print_colored("\nYou carefully open the chest...\n", make_color(LIGHT_GREY, BLACK));
        print_colored("*FLASH* A burst of light blinds you momentarily!\n",
                      make_color(WHITE, BLACK));
        print_colored("You found a magical sword and 100 gold coins!\n",
                      make_color(LIGHT_GREEN, BLACK));
    } else {
        print_colored("You decide to play it safe and leave the chest alone.\n",
                      make_color(LIGHT_RED, BLACK));
        print_colored("Perhaps some mysteries are better left unsolved...\n",
                      make_color(MAGENTA, BLACK));
    }
    print_colored("\nPress any key to continue...\n", make_color(WHITE, BLACK));
    get_keyboard_char();
}

void adventure_south() {
    clear_screen();
    print_colored("You venture south into darker woods...\n", make_color(DARK_GREY, BLACK));
    print_colored("Suddenly, a massive dragon appears before you!\n", make_color(RED, BLACK));
    print_colored("Its scales shimmer with an otherworldly glow...\n",
                  make_color(LIGHT_RED, BLACK));
    print_colored("Do you want to fight it? (y/n): ", make_color(WHITE, BLACK));

    char choice[2];
    read_line(choice, sizeof(choice));

    if (strcmp(choice, "y") == 0) {
        print_colored("\nYou draw your weapon and charge forward!\n",
                      make_color(LIGHT_BROWN, BLACK));
        print_colored("After an epic battle, you emerge victorious!\n", make_color(GREEN, BLACK));
        print_colored("The dragon transforms into a friendly spirit...\n",
                      make_color(LIGHT_CYAN, BLACK));
    } else {
        print_colored("You wisely choose to retreat...\n", make_color(LIGHT_BLUE, BLACK));
        print_colored("The dragon nods respectfully at your decision.\n", make_color(CYAN, BLACK));
    }
    print_colored("\nPress any key to continue...\n", make_color(WHITE, BLACK));
    get_keyboard_char();
}

void adventure_east() {
    clear_screen();
    print_colored("You travel east and discover a mystical village!\n",
                  make_color(LIGHT_GREEN, BLACK));
    print_colored("An old sage approaches you with ancient wisdom...\n", make_color(CYAN, BLACK));
    print_colored("Do you want to hear their counsel? (y/n): ", make_color(WHITE, BLACK));

    char choice[2];
    read_line(choice, sizeof(choice));

    if (strcmp(choice, "y") == 0) {
        print_colored("\nThe sage reveals secrets of great power...\n", make_color(MAGENTA, BLACK));
        print_colored("You learn about a legendary artifact!\n", make_color(LIGHT_BROWN, BLACK));
        print_colored("This knowledge will serve you well...\n", make_color(LIGHT_CYAN, BLACK));
    } else {
        print_colored("You politely decline the sage's offer.\n", make_color(LIGHT_GREY, BLACK));
        print_colored("Sometimes ignorance is bliss...\n", make_color(DARK_GREY, BLACK));
    }
    print_colored("\nPress any key to continue...\n", make_color(WHITE, BLACK));
    get_keyboard_char();
}

void adventure_west() {
    clear_screen();
    print_colored("You head west into a mysterious fog...\n", make_color(LIGHT_BLUE, BLACK));
    print_colored("The mists swirl around you creating strange shapes...\n",
                  make_color(CYAN, BLACK));
    print_colored("You hear whispers from the beyond...\n", make_color(MAGENTA, BLACK));
    print_colored("As the fog clears, you find your way back...\n", make_color(GREEN, BLACK));
    print_colored("But you're not quite the same as before...\n", make_color(LIGHT_MAGENTA, BLACK));
    print_colored("\nPress any key to continue...\n", make_color(WHITE, BLACK));
    get_keyboard_char();
}

void cpuinfo() {
    char vendor[13];
    uint32_t eax, ebx, ecx, edx;

    // Get vendor string
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);

    *(uint32_t *)(&vendor[0]) = ebx;
    *(uint32_t *)(&vendor[4]) = edx;
    *(uint32_t *)(&vendor[8]) = ecx;
    vendor[12] = '\0';

    print("CPU Vendor: ");
    print(vendor);
    print("\n");

    // Get CPU features
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    print("CPU Features:\n");
    if (edx & (1 << 0)) print("- FPU\n");
    if (edx & (1 << 4)) print("- TSC\n");
    if (edx & (1 << 5)) print("- MSR\n");
    if (edx & (1 << 23)) print("- MMX\n");
    if (edx & (1 << 25)) print("- SSE\n");
    if (edx & (1 << 26)) print("- SSE2\n");
    if (ecx & (1 << 0)) print("- SSE3\n");
}

// Modified time function
void time() {
    uint8_t second, minute, hour;

    // Disable NMI
    outb(0x70, 0x80);

    // Read CMOS registers
    outb(0x70, 0x00);
    second = inb(0x71);
    outb(0x70, 0x02);
    minute = inb(0x71);
    outb(0x70, 0x04);
    hour = inb(0x71);

    // Re-enable NMI
    outb(0x70, 0x00);

    // Convert BCD to binary
    second = ((second & 0xF0) >> 4) * 10 + (second & 0x0F);
    minute = ((minute & 0xF0) >> 4) * 10 + (minute & 0x0F);
    hour = ((hour & 0xF0) >> 4) * 10 + (hour & 0x0F);

    // Print time
    print("Current time: ");
    if (hour < 10) putchar('0');
    char buffer[3];
    int i = 0;

    // Convert hour
    do {
        buffer[i++] = (hour % 10) + '0';
        hour /= 10;
    } while (hour > 0);
    while (i > 0) putchar(buffer[--i]);

    print(":");

    // Convert minute
    if (minute < 10) putchar('0');
    i = 0;
    do {
        buffer[i++] = (minute % 10) + '0';
        minute /= 10;
    } while (minute > 0);
    while (i > 0) putchar(buffer[--i]);

    print(":");

    // Convert second
    if (second < 10) putchar('0');
    i = 0;
    do {
        buffer[i++] = (second % 10) + '0';
        second /= 10;
    } while (second > 0);
    while (i > 0) putchar(buffer[--i]);

    print("\n");
}

void reboot() {
    print("Rebooting NoirOS...\n");
    print("Please wait while the system restarts...\n");

    // Wait for keyboard buffer to empty
    uint8_t temp;
    do {
        temp = inb(KEYBOARD_STATUS);
        if (temp & 1) {
            inb(KEYBOARD_PORT);
        }
    } while (temp & 2);

    // Send reset command
    outb(KEYBOARD_STATUS, 0xFE);

    // If that didn't work, try alternative method
    while (1) {
        asm volatile("hlt");
    }
}

void echo(const char *str) {
    print(str);
    print("\n");
}

void print_system_info() {
    print("NoirOS v1.0\n");
    print("Architecture: x86\n");
    print("Memory: 640KB Base Memory\n");
    print("Display: VGA Text Mode 80x26\n");
}

void whoami() {
    print("root\n");
}

void hostname() {
    print("noiros\n");
}

static uint32_t next = 1;  // Seed for the random number generator

// Simple pseudo-random number generator
static uint32_t z1 = 12345, z2 = 67890, z3 = 11111, z4 = 22222;

// Xorshift algorithm combined with Linear Congruential Generator
uint32_t rand() {
    // Get some system entropy from timer and other sources
    uint8_t timer_low = inb(0x40);        // Timer counter low byte
    uint8_t timer_high = inb(0x40);       // Timer counter high byte
    uint8_t keyboard_status = inb(0x64);  // Keyboard controller status
    uint32_t timer_value = (timer_high << 8) | timer_low;

    // Xorshift algorithm
    z1 ^= (z1 << 11);
    z1 ^= (z1 >> 8);
    z2 ^= (z2 << 13);
    z2 ^= (z2 >> 17);
    z3 ^= (z3 << 9);
    z3 ^= (z3 >> 7);
    z4 = z4 ^ (z4 << 15);

    // Combine multiple sources of randomness
    uint32_t result = (z1 ^ z2 ^ z3 ^ z4) + timer_value + keyboard_status;

    // Linear congruential generator
    next = next * 1664525 + 1013904223;

    // Mix everything together
    result ^= next;
    result ^= (result << 13);
    result ^= (result >> 17);
    result ^= (result << 5);

    return result;
}

void srand(uint32_t seed) {
    // Initialize all state variables with different transformations of the seed
    next = seed;
    z1 = seed ^ 0x12345678;
    z2 = seed ^ 0x87654321;
    z3 = seed ^ 0xFEDCBA98;
    z4 = seed ^ 0x11223344;

    // Additional mixing
    for (int i = 0; i < 10; i++) {
        rand();  // Discard first few values to improve distribution
    }
}

// Helper function to get random number in a specific range
uint32_t rand_range(uint32_t min, uint32_t max) {
    uint32_t range = max - min + 1;
    return min + (rand() % range);
}

void fortune() {
    const char *fortunes[] = {"The best way to predict the future is to invent it.",
                              "Stay hungry, stay foolish.",
                              "The only way to do great work is to love what you do.",
                              "Innovation distinguishes between a leader and a follower.",
                              "The journey of a thousand miles begins with one step.",
                              "Life is 10% what happens to us and 90% how we react to it.",
                              "Your time is limited, don't waste it living someone else's life.",
                              "You only live once, but if you do it right, once is enough."};
    int fortune_count = sizeof(fortunes) / sizeof(fortunes[0]);
    int random_index = rand() % fortune_count;  // Get a random index

    print_colored("Your fortune: \n", make_color(LIGHT_CYAN, BLACK));
    print_colored(fortunes[random_index], make_color(LIGHT_GREEN, BLACK));  // Display the fortune
    print("\n");
    print("\n");
}

// Function prototypes for the adventure game
void adventure_north(void);
void adventure_south(void);
void adventure_east(void);
void adventure_west(void);
void textadventure(void);

void print_banner() {
    print("    _   __      _      ____  _____ \n");
    print("   / | / /___  (_)____/ __ \\/ ___/ \n");
    print("  /  |/ / __ \\/ / ___/ / / /\\__ \\ \n");
    print(" / /|  / /_/ / / /  / /_/ /___/ / \n");
    print("/_/ |_/\\____/_/_/   \\____//____/  \n");
    print_colored("\nWelcome to NoirOS!\n", make_color(LIGHT_CYAN, BLACK));
}

#define MAX_LINES 100
#define MAX_LINE_LENGTH 80

char text_buffer[MAX_LINES][MAX_LINE_LENGTH];
int current_line = 0;
int num_lines = 0;

void noirtext(const char* filename) {
    clear_screen();
    print_colored("Welcome to NoirText!\n", make_color(LIGHT_CYAN, BLACK));
    print_colored("Commands: :w to save, :q to quit\n\n", make_color(LIGHT_GREEN, BLACK));

    // Load file content if it exists
    FileEntry* file = NULL;
    if (filename) {
        for (int i = 0; i < fs.current_dir->num_files; i++) {
            if (strcmp(fs.current_dir->files[i].filename, filename) == 0) {
                file = &fs.current_dir->files[i];
                break;
            }
        }
        
        if (file) {
            char* content = (char*)file->start_block;
            int line = 0;
            int col = 0;
            for (size_t i = 0; i < file->size && line < MAX_LINES; i++) {
                if (content[i] == '\n' || col == MAX_LINE_LENGTH - 1) {
                    text_buffer[line][col] = '\0';
                    line++;
                    col = 0;
                } else {
                    text_buffer[line][col] = content[i];
                    col++;
                }
            }
            if (col > 0) {
                text_buffer[line][col] = '\0';
                line++;
            }
            num_lines = line;
        }
    }

    while (1) {
        // Display current content
        for (int i = 0; i < num_lines; i++) {
            print(text_buffer[i]);
            print("\n");
        }

        // Get user input
        char input[MAX_LINE_LENGTH];
        print("> ");
        read_line(input, MAX_LINE_LENGTH);

        // Check for commands
        if (strcmp(input, ":w") == 0) {
            // Save file
            if (filename) {
                if (file == NULL) {
                    if (fs.current_dir->num_files >= MAX_FILES) {
                        print_colored("Error: Directory is full\n", make_color(LIGHT_RED, BLACK));
                        continue;
                    }
                    file = &fs.current_dir->files[fs.current_dir->num_files];
                    strncpy(file->filename, filename, MAX_FILENAME);
                    fs.current_dir->num_files++;
                }

                // Calculate total content size
                size_t total_size = 0;
                for (int i = 0; i < num_lines; i++) {
                    total_size += strlen(text_buffer[i]) + 1; // +1 for newline
                }

                // Allocate memory for file content
                char* content = (char*)malloc(total_size + 1); // +1 for null terminator
                if (content == NULL) {
                    print_colored("Error: Failed to allocate memory for file content\n", make_color(LIGHT_RED, BLACK));
                    continue;
                }

                // Copy content to file
                char* ptr = content;
                for (int i = 0; i < num_lines; i++) {
                    size_t line_len = strlen(text_buffer[i]);
                    memcpy(ptr, text_buffer[i], line_len);
                    ptr += line_len;
                    *ptr++ = '\n';
                }
                *ptr = '\0'; // Null terminate the content

                // Update file entry
                if (file->start_block != 0) {
                    free((void*)file->start_block);
                }
                file->start_block = (uint32_t)content;
                file->size = total_size;

                print_colored("File saved.\n", make_color(LIGHT_GREEN, BLACK));
            } else {
                print_colored("No filename specified.\n", make_color(LIGHT_RED, BLACK));
            }
            continue;
        } else if (strcmp(input, ":q") == 0) {
            // Quit
            break;
        }

        // Add new line to buffer
        if (num_lines < MAX_LINES) {
            strncpy(text_buffer[num_lines], input, MAX_LINE_LENGTH);
            num_lines++;
        } else {
            print_colored("Buffer full!\n", make_color(LIGHT_RED, BLACK));
        }

        clear_screen();
    }

    clear_screen();
}

void execute_command(const char *command) {
    if (strcmp(command, "clear") == 0) {
        clear_screen();
    } else if (strcmp(command, "help") == 0) {
        print_colored("Available commands:\n", make_color(LIGHT_CYAN, BLACK));
        print("  clear    - Clear the screen       | help     - Show this help message\n");
        print("  shutdown - Power off the system   | reboot   - Restart the system\n");
        print("  echo [text] - Display the text    | whoami   - Display current user\n");
        print("  hostname - Display system hostname| uname    - Display system information\n");
        print("  banner   - Display NoirOS banner  | cpuinfo  - Display CPU information\n");
        print("  time     - Display current time   | calc [expression] - Basic calculator\n");
        print("  textgame - Start a game           | play     - Play a silly tune\n");
        print("  fortune  - Display a fortune.     | touch    - Create a file.\n");
        print("  cat      - Show contents of file  | mkdir    - Create a directory\n");
        print("  ls       - List files and dirs    | cd       - Change directory \n");
    } else if (strcmp(command, "shutdown") == 0) {
        shutdown();
    } else if (strcmp(command, "reboot") == 0) {
        reboot();
    } else if (strncmp(command, "echo ", 5) == 0) {
        echo(command + 5);
    } else if (strcmp(command, "whoami") == 0) {
        whoami();
    } else if (strcmp(command, "hostname") == 0) {
        hostname();
    } else if (strcmp(command, "uname") == 0) {
        print_system_info();
    } else if (strcmp(command, "banner") == 0) {
        print_banner();
    } else if (strcmp(command, "cpuinfo") == 0) {
        cpuinfo();
    } else if (strcmp(command, "time") == 0) {
        time();
    } else if (strncmp(command, "calc ", 5) == 0) {
        calc(command + 5);
    } else if (strncmp(command, "textgame", 5) == 0) {
        textadventure();
    } else if (strcmp(command, "play") == 0) {
        play_silly_tune();
    } else if (strcmp(command, "fortune") == 0) {
        fortune();
    } else if (strncmp(command, "touch ", 6) == 0) {
        touch(command + 6);
    } else if (strncmp(command, "cat ", 4) == 0) {
        cat(command + 4);
    } else if (strncmp(command, "mkdir ", 6) == 0) {
        mkdir(command + 6);
    } else if (strcmp(command, "ls") == 0) {
        ls();
    } else if (strncmp(command, "cd ", 3) == 0) {
        cd(command + 3);
    } else if (strncmp(command, "noirtext ", 9) == 0) {
        noirtext(command + 9);  // Pass filename if provided
    } else if (strcmp(command, "noirtext") == 0) {
        noirtext(NULL);  // No filename provided
    } else {
        print("Unknown command: ");
        print(command);
        print("\n");
    }
}

void shell() {
    char command[256];
    while (1) {
        print_colored("root", make_color(LIGHT_GREEN, BLACK));
        print_colored("@", make_color(WHITE, BLACK));
        print_colored("noiros", make_color(LIGHT_CYAN, BLACK));
        print_colored(" # ", make_color(LIGHT_RED, BLACK));
        read_line(command, sizeof(command));
        execute_command(command);
    }
}

int kernel_main() {
    clear_screen();
    print_banner();
    init_fs();
    print_colored("Type 'help' for a list of commands.\n\n", make_color(LIGHT_MAGENTA, BLACK));
    shell();
    return 0;
}