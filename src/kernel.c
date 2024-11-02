// kernel.c
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t;
typedef int bool;
#define true 1
#define false 0

// VGA constants
#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define VGA_MEMORY 0xA0000

// Colors
#define COLOR_BLACK 0x00
#define COLOR_BLUE 0x01
#define COLOR_GREEN 0x02
#define COLOR_CYAN 0x03
#define COLOR_RED 0x04
#define COLOR_MAGENTA 0x05
#define COLOR_BROWN 0x06
#define COLOR_LIGHT_GRAY 0x07
#define COLOR_DARK_GRAY 0x08
#define COLOR_LIGHT_BLUE 0x09
#define COLOR_LIGHT_GREEN 0x0A
#define COLOR_LIGHT_CYAN 0x0B
#define COLOR_LIGHT_RED 0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW 0x0E
#define COLOR_WHITE 0x0F

// VGA framebuffer
static uint8_t* vga_buffer = (uint8_t*)VGA_MEMORY;

// Window structure
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    const char* title;
    bool active;
} Window;

// Port IO functions
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Switch to VGA mode 13h (320x200x256)
void init_graphics() {
    outb(0x3C8, 0);  // Set palette index to 0
    
    // Set up a grayscale palette for now
    for(int i = 0; i < 16; i++) {
        outb(0x3C9, i * 4);  // Red
        outb(0x3C9, i * 4);  // Green
        outb(0x3C9, i * 4);  // Blue
    }
    
    // Switch to mode 13h
    outb(0x3C0, 0x10); // Mode control register
    outb(0x3C0, 0x41); // Enable graphics mode

    outb(0x3C0, 0x20); // Enable display
    
    // Clear screen
    for(int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = COLOR_LIGHT_GRAY;
    }
}

// Basic drawing functions
void draw_pixel(int x, int y, uint8_t color) {
    if(x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_buffer[y * VGA_WIDTH + x] = color;
    }
}

void draw_rect(int x, int y, int width, int height, uint8_t color) {
    for(int i = x; i < x + width; i++) {
        for(int j = y; j < y + height; j++) {
            draw_pixel(i, j, color);
        }
    }
}

void draw_border(int x, int y, int width, int height, uint8_t color) {
    // Top and bottom
    for(int i = x; i < x + width; i++) {
        draw_pixel(i, y, color);
        draw_pixel(i, y + height - 1, color);
    }
    // Left and right
    for(int i = y; i < y + height; i++) {
        draw_pixel(x, i, color);
        draw_pixel(x + width - 1, i, color);
    }
}

// Window management
void draw_window(Window* win) {
    // Draw window background
    draw_rect(win->x, win->y, win->width, win->height, COLOR_LIGHT_GRAY);
    
    // Draw window border
    draw_border(win->x, win->y, win->width, win->height, COLOR_BLACK);
    
    // Draw title bar
    draw_rect(win->x, win->y, win->width, 20, win->active ? COLOR_BLUE : COLOR_DARK_GRAY);
    
    // Draw title bar bottom border
    draw_rect(win->x, win->y + 19, win->width, 1, COLOR_BLACK);
}

// Test windows
Window windows[] = {
    {10, 10, 200, 150, "NoirOS", true},
    {50, 50, 200, 150, "Window 2", false}
};

void kernel_main(void) {
    // Switch to graphics mode
    init_graphics();
    
    // Draw some test windows
    for(size_t i = 0; i < sizeof(windows) / sizeof(Window); i++) {
        draw_window(&windows[i]);
    }
    
    // Main loop (empty for now)
    while(1) {
        // TODO: Add event handling
    }
}