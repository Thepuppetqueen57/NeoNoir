#define VIDEO_MEMORY_START 0xb8000
#define VIDEO_MEMORY_SIZE 4800

char* video_memory = (char*) VIDEO_MEMORY_START;

void video_init() {
    for (int i = 0; i < VIDEO_MEMORY_SIZE; i += 2) {
        video_memory[i] = ' ';
        video_memory[i + 1] = 0x07;
    }
}

void console_write(const char* str) {
    int i = 0;
    while (str[i] != '\0' && i * 2 < VIDEO_MEMORY_SIZE) {
        video_memory[i * 2] = str[i];
        video_memory[i * 2 + 1] = 0x07;
        i++;
    }
}

void cpu_halt() {
    __asm__("hlt");
}

int kernel_main() {
    video_init();
    console_write("Initialized kernel.\n");
    while (1) {
        cpu_halt();
    }
    return 0;
}