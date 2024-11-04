#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

#define KEYBOARD_PORT 0x60
#define KEYBOARD_STATUS 0x64
#define SHUTDOWN_PORT1 0xB004
#define SHUTDOWN_PORT2 0x2000
#define SHUTDOWN_PORT3 0x604

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
int cursor_x = 0;
int cursor_y = 0;

// IO functions
void
outb (uint16_t port, uint8_t val)
{
  asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void
outw (uint16_t port, uint16_t val)
{
  asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t
inb (uint16_t port)
{
  uint8_t ret;
  asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

int
strcmp (const char *s1, const char *s2)
{
  while (*s1 && (*s1 == *s2))
    {
      s1++;
      s2++;
    }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int
strlen (const char *str)
{
  int len = 0;
  while (str[len])
    len++;
  return len;
}

int
strncmp (const char *s1, const char *s2, int n)
{
  while (n && *s1 && (*s1 == *s2))
    {
      ++s1;
      ++s2;
      --n;
    }
  if (n == 0)
    return 0;
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// VGA entry color
enum vga_color
{
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

uint8_t
make_color (enum vga_color fg, enum vga_color bg)
{
  return fg | bg << 4;
}

void
update_cursor ()
{
  uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;

  outb (0x3D4, 0x0F);
  outb (0x3D5, (uint8_t)(pos & 0xFF));
  outb (0x3D4, 0x0E);
  outb (0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

#define LSHIFT 0x2A
#define RSHIFT 0x36
#define CAPS_LOCK 0x3A

char
get_keyboard_char ()
{
  static const char sc_ascii[] = {
    0,   27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
    '-', '=', '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    'o', 'p',  '[',  ']', '\n', 0,    'a',  's',  'd',  'f',  'g',  'h',
    'j', 'k',  'l',  ';', '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    'b', 'n',  'm',  ',', '.',  '/',  0,    '*',  0,    ' '
  };

  static const char sc_ascii_shift[] = {
    0,   27,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
    '_', '+', '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    'O', 'P',  '{',  '}', '\n', 0,    'A',  'S',  'D',  'F',  'G',  'H',
    'J', 'K',  'L',  ':', '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B', 'N',  'M',  '<', '>',  '?',  0,    '*',  0,    ' '
  };

  static int shift = 0;
  static int caps_lock = 0;

  while (1)
    {
      if (inb (0x64) & 0x1)
        {
          uint8_t scancode = inb (0x60);
          
          if (scancode == LSHIFT || scancode == RSHIFT)
            {
              shift = 1;
              continue;
            }
          else if (scancode == (LSHIFT | 0x80) || scancode == (RSHIFT | 0x80))
            {
              shift = 0;
              continue;
            }
          else if (scancode == CAPS_LOCK)
            {
              caps_lock = !caps_lock;
              continue;
            }

          if (!(scancode & 0x80))
            {
              if (scancode < sizeof (sc_ascii))
                {
                  char c;
                  if (shift)
                    {
                      c = sc_ascii_shift[scancode];
                    }
                  else
                    {
                      c = sc_ascii[scancode];
                    }

                  if (caps_lock && c >= 'a' && c <= 'z')
                    {
                      c -= 32;  // Convert to uppercase
                    }
                  else if (caps_lock && c >= 'A' && c <= 'Z')
                    {
                      c += 32;  // Convert to lowercase
                    }

                  if (c)
                    {
                      return c;
                    }
                }
            }
        }

      // Small delay to prevent overwhelming the CPU
      for (volatile int i = 0; i < 10000; i++)
        {
        }
    }
}

uint16_t
make_vga_entry (char c, uint8_t color)
{
  return (uint16_t)c | (uint16_t)color << 8;
}

void
clear_screen ()
{
  for (int y = 0; y < VGA_HEIGHT; y++)
    {
      for (int x = 0; x < VGA_WIDTH; x++)
        {
          const int index = y * VGA_WIDTH + x;
          vga_buffer[index] = make_vga_entry (' ', make_color (WHITE, BLACK));
        }
    }
  cursor_x = 0;
  cursor_y = 0;
  update_cursor ();
}

void cpuinfo ();
void time ();
void calc (const char *expression);

void
putchar (char c)
{
  if (c == '\n')
    {
      cursor_x = 0;
      cursor_y++;
    }
  else if (c == '\b')
    {
      if (cursor_x > 0)
        {
          cursor_x--;
          const int index = cursor_y * VGA_WIDTH + cursor_x;
          vga_buffer[index] = make_vga_entry (' ', make_color (WHITE, BLACK));
        }
    }
  else
    {
      const int index = cursor_y * VGA_WIDTH + cursor_x;
      vga_buffer[index] = make_vga_entry (c, make_color (WHITE, BLACK));
      cursor_x++;
    }

  if (cursor_x >= VGA_WIDTH)
    {
      cursor_x = 0;
      cursor_y++;
    }

  if (cursor_y >= VGA_HEIGHT)
    {
      // Scroll the screen
      for (int y = 0; y < VGA_HEIGHT - 1; y++)
        {
          for (int x = 0; x < VGA_WIDTH; x++)
            {
              vga_buffer[y * VGA_WIDTH + x]
                  = vga_buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
      // Clear the last line
      for (int x = 0; x < VGA_WIDTH; x++)
        {
          vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x]
              = make_vga_entry (' ', make_color (WHITE, BLACK));
        }
      cursor_y = VGA_HEIGHT - 1;
    }
  update_cursor ();
}

void
print (const char *str)
{
  while (*str)
    {
      putchar (*str);
      str++;
    }
}

void
read_line (char *buffer, int max_length)
{
  int i = 0;
  char c;
  while (i < max_length - 1)
    {
      c = get_keyboard_char ();
      if (c == '\n')
        {
          buffer[i] = '\0';
          putchar ('\n');
          return;
        }
      else if (c == '\b' && i > 0)
        {
          i--;
          putchar ('\b');
        }
      else if (c >= 32 && c <= 126)
        {
          buffer[i] = c;
          putchar (c);
          i++;
        }
    }
  buffer[i] = '\0';
}

void
shutdown ()
{
  print ("Shutting down NoirOS...\n");
  print ("Please wait while the system powers off...\n");

  // Try ACPI shutdown
  outw (SHUTDOWN_PORT1, 0x2000);

  // Try APM shutdown
  outw (SHUTDOWN_PORT2, 0x0);

  // Try another method
  outw (SHUTDOWN_PORT3, 0x2000);

  // If we get here , shutdown failed
  print ("Shutdown failed. System halted.\n");
  while (1)
    {
      asm volatile ("hlt");
    }
}

// Add these type definitions if not already present
typedef unsigned int uint32_t;
typedef int int32_t;

// String parsing function for the calculator
int
parse_number (const char **str)
{
  int num = 0;
  int sign = 1;

  // Skip whitespace
  while (**str == ' ')
    (*str)++;

  // Handle negative numbers
  if (**str == '-')
    {
      sign = -1;
      (*str)++;
    }

  while (**str >= '0' && **str <= '9')
    {
      num = num * 10 + (**str - '0');
      (*str)++;
    }

  return num * sign;
}

char
parse_operator (const char **str)
{
  // Skip whitespace
  while (**str == ' ')
    (*str)++;

  char op = **str;
  if (op)
    (*str)++;

  return op;
}

// Modified calculator function
void
calc (const char *expression)
{
  const char *ptr = expression;
  int a, b, result;
  char op;

  // Parse first number
  a = parse_number (&ptr);

  // Parse operator
  op = parse_operator (&ptr);

  // Parse second number
  b = parse_number (&ptr);

  switch (op)
    {
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
      if (b == 0)
        {
          print ("Error: Division by zero\n");
          return;
        }
      result = a / b;
      break;
    default:
      print ("Invalid operator. Supported operators: +, -, *, /\n");
      return;
    }

  // Print result using our own number to string conversion
  char buffer[20];
  int i = 0;
  int is_negative = 0;
  int temp;

  // Convert first number
  if (a < 0)
    {
      putchar ('-');
      a = -a;
    }
  temp = a;
  do
    {
      buffer[i++] = (temp % 10) + '0';
      temp /= 10;
    }
  while (temp > 0);
  while (i > 0)
    putchar (buffer[--i]);

  // Print operator
  putchar (' ');
  putchar (op);
  putchar (' ');

  // Convert second number
  if (b < 0)
    {
      putchar ('-');
      b = -b;
    }
  temp = b;
  do
    {
      buffer[i++] = (temp % 10) + '0';
      temp /= 10;
    }
  while (temp > 0);
  while (i > 0)
    putchar (buffer[--i]);

  print (" = ");

  // Convert result
  if (result < 0)
    {
      putchar ('-');
      result = -result;
    }
  temp = result;
  do
    {
      buffer[i++] = (temp % 10) + '0';
      temp /= 10;
    }
  while (temp > 0);
  while (i > 0)
    putchar (buffer[--i]);

  print ("\n");
}

// Modified cpuinfo function
// Function to emulate CPUID instruction
static inline void
cpuid (uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx,
       uint32_t *ecx, uint32_t *edx)
{
  __asm__ __volatile__ ("cpuid"
                        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                        : "a"(leaf), "c"(subleaf));
}

void
cpuinfo ()
{
  char vendor[13];
  uint32_t eax, ebx, ecx, edx;

  // Get vendor string
  cpuid (0, 0, &eax, &ebx, &ecx, &edx);

  *(uint32_t *)(&vendor[0]) = ebx;
  *(uint32_t *)(&vendor[4]) = edx;
  *(uint32_t *)(&vendor[8]) = ecx;
  vendor[12] = '\0';

  print ("CPU Vendor: ");
  print (vendor);
  print ("\n");

  // Get CPU features
  cpuid (1, 0, &eax, &ebx, &ecx, &edx);

  print ("CPU Features:\n");
  if (edx & (1 << 0))
    print ("- FPU\n");
  if (edx & (1 << 4))
    print ("- TSC\n");
  if (edx & (1 << 5))
    print ("- MSR\n");
  if (edx & (1 << 23))
    print ("- MMX\n");
  if (edx & (1 << 25))
    print ("- SSE\n");
  if (edx & (1 << 26))
    print ("- SSE2\n");
  if (ecx & (1 << 0))
    print ("- SSE3\n");
}

// Modified time function
void
time ()
{
  uint8_t second, minute, hour;

  // Disable NMI
  outb (0x70, 0x80);

  // Read CMOS registers
  outb (0x70, 0x00);
  second = inb (0x71);
  outb (0x70, 0x02);
  minute = inb (0x71);
  outb (0x70, 0x04);
  hour = inb (0x71);

  // Re-enable NMI
  outb (0x70, 0x00);

  // Convert BCD to binary
  second = ((second & 0xF0) >> 4) * 10 + (second & 0x0F);
  minute = ((minute & 0xF0) >> 4) * 10 + (minute & 0x0F);
  hour = ((hour & 0xF0) >> 4) * 10 + (hour & 0x0F);

  // Print time
  print ("Current time: ");
  if (hour < 10)
    putchar ('0');
  char buffer[3];
  int i = 0;

  // Convert hour
  do
    {
      buffer[i++] = (hour % 10) + '0';
      hour /= 10;
    }
  while (hour > 0);
  while (i > 0)
    putchar (buffer[--i]);

  print (":");

  // Convert minute
  if (minute < 10)
    putchar ('0');
  i = 0;
  do
    {
      buffer[i++] = (minute % 10) + '0';
      minute /= 10;
    }
  while (minute > 0);
  while (i > 0)
    putchar (buffer[--i]);

  print (":");

  // Convert second
  if (second < 10)
    putchar ('0');
  i = 0;
  do
    {
      buffer[i++] = (second % 10) + '0';
      second /= 10;
    }
  while (second > 0);
  while (i > 0)
    putchar (buffer[--i]);

  print ("\n");
}

void
reboot ()
{
  print ("Rebooting NoirOS...\n");
  print ("Please wait while the system restarts...\n");

  // Wait for keyboard buffer to empty
  uint8_t temp;
  do
    {
      temp = inb (KEYBOARD_STATUS);
      if (temp & 1)
        {
          inb (KEYBOARD_PORT);
        }
    }
  while (temp & 2);

  // Send reset command
  outb (KEYBOARD_STATUS, 0xFE);

  // If that didn't work, try alternative method
  while (1)
    {
      asm volatile ("hlt");
    }
}

void
echo (const char *str)
{
  print (str);
  print ("\n");
}

void
print_system_info ()
{
  print ("NoirOS v1.0\n");
  print ("Architecture: x86\n");
  print ("Memory: 640KB Base Memory\n");
  print ("Display: VGA Text Mode 80x25\n");
}

void
whoami ()
{
  print ("root\n");
}

void
hostname ()
{
  print ("noiros\n");
}

void
print_banner ()
{
  print ("    _   __      _      ____  _____ \n");
  print ("   / | / /___  (_)____/ __ \\/ ___/ \n");
  print ("  /  |/ / __ \\/ / ___/ / / /\\__ \\ \n");
  print (" / /|  / /_/ / / /  / /_/ /___/ / \n");
  print ("/_/ |_/\\____/_/_/   \\____//____/  \n");
  print ("\nWelcome to NoirOS!\n");
}

void
execute_command (const char *command)
{
  if (strcmp (command, "clear") == 0)
    {
      clear_screen ();
    }
  else if (strcmp (command, "help") == 0)
    {
      print ("Available commands:\n");
      print ("  clear    - Clear the screen\n");
      print ("  help     - Show this help message\n");
      print ("  shutdown - Power off the system\n");
      print ("  reboot   - Restart the system\n");
      print ("  echo [text] - Display the text\n");
      print ("  whoami   - Display current user\n");
      print ("  hostname - Display system hostname\n");
      print ("  uname    - Display system information\n");
      print ("  banner   - Display NoirOS banner\n");
      print ("  cpuinfo  - Display CPU information\n");
      print ("  time     - Display current time\n");
      print ("  calc [expression] - Basic calculator\n");
    }
  else if (strcmp (command, "shutdown") == 0)
    {
      shutdown ();
    }
  else if (strcmp (command, "reboot") == 0)
    {
      reboot ();
    }
  else if (strncmp (command, "echo ", 5) == 0)
    {
      echo (command + 5);
    }
  else if (strcmp (command, "whoami") == 0)
    {
      whoami ();
    }
  else if (strcmp (command, "hostname") == 0)
    {
      hostname ();
    }
  else if (strcmp (command, "uname") == 0)
    {
      print_system_info ();
    }
  else if (strcmp (command, "banner") == 0)
    {
      print_banner ();
    }
  else if (strcmp (command, "cpuinfo") == 0)
    {
      cpuinfo ();
    }
  else if (strcmp (command, "time") == 0)
    {
      time ();
    }
  else if (strncmp (command, "calc ", 5) == 0)
    {
      calc (command + 5);
    }
  else
    {
      print ("Unknown command: ");
      print (command);
      print ("\n");
    }
}

void
shell ()
{
  char command[256];
  while (1)
    {
      print ("root@noiros /> ");
      read_line (command, sizeof (command));
      execute_command (command);
    }
}

int
kernel_main ()
{
  clear_screen ();
  print_banner ();
  print ("Type 'help' for a list of commands.\n\n");
  shell ();
  return 0;
}