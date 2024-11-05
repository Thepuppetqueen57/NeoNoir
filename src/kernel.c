#include "noirlib.c"

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