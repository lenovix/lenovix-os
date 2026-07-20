#include "keyboard.h"
#include "tty.h"

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);

volatile char last_ascii_char = 0;

// Import dari shell untuk interaksi dengan command buffer & history
extern char command_buffer[256];
extern int command_index;
extern void process_command(void);
extern void handle_history_up(void);
extern void handle_history_down(void);

unsigned char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

unsigned char scancode_to_ascii_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0,
  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0, '*',   0, ' '
};

static int shift_pressed = 0;
static int caps_lock_active = 0;
static int is_extended = 0;

char get_last_key(void) {
    char c = last_ascii_char;
    last_ascii_char = 0;
    return c;
}

void keyboard_handler_c(void) {
    unsigned char scancode = inb(0x60);

    if (scancode == 0xE0) {
        is_extended = 1;
        outb(0x20, 0x20);
        return;
    }

    if (is_extended) {
        is_extended = 0;
        if (scancode == 0x48) handle_history_up();   // Arrow Up
        else if (scancode == 0x50) handle_history_down(); // Arrow Down
        outb(0x20, 0x20);
        return;
    }

    if (scancode == 0x2A || scancode == 0x36) shift_pressed = 1;
    else if (scancode == 0xAA || scancode == 0xB6) shift_pressed = 0;
    else if (scancode == 0x3A) caps_lock_active = !caps_lock_active;
    else if (scancode < 0x80) {
        unsigned char ascii = 0;
        int is_letter = (scancode >= 0x10 && scancode <= 0x19) ||
                        (scancode >= 0x1E && scancode <= 0x26) ||
                        (scancode >= 0x2C && scancode <= 0x32);

        if (is_letter) {
            ascii = (caps_lock_active ^ shift_pressed) ? 
                    scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
        } else {
            ascii = shift_pressed ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
        }
        
        if (ascii != 0) {
            last_ascii_char = ascii; // Update last key for syscall
            if (ascii == '\n') {
                process_command();
            } else if (ascii == '\b') {
                if (command_index > 0) {
                    command_index--;
                    kprint_char(ascii, 0x0F);
                }
            } else {
                if (command_index < 255) {
                    command_buffer[command_index] = ascii;
                    command_index++;
                    kprint_char(ascii, 0x0F);
                }
            }
        }
    }

    outb(0x20, 0x20);
}