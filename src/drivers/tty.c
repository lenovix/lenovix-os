#include "tty.h"

int cursor_x = 0;
int cursor_y = 0;
char *video_memory = (char *) 0xB8000;

extern void outb(unsigned short port, unsigned char data);

void update_hardware_cursor(void) {
    unsigned short position = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = 0x07;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_hardware_cursor();
}

static void scroll(void) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            int target_index = ((y - 1) * VGA_WIDTH + x) * 2;
            int source_index = (y * VGA_WIDTH + x) * 2;
            video_memory[target_index] = video_memory[source_index];
            video_memory[target_index + 1] = video_memory[source_index + 1];
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        int index = ((VGA_HEIGHT - 1) * VGA_WIDTH + x) * 2;
        video_memory[index] = ' ';
        video_memory[index + 1] = 0x07;
    }

    cursor_y = VGA_HEIGHT - 1;
}

void kprint_char(char c, char color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3; 
    } else if (c == '\b') {
        if (cursor_x > 2) {
            cursor_x--;
            int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
            video_memory[index] = ' ';
            video_memory[index + 1] = color;
        }
    } else {
        int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
        video_memory[index] = c;
        video_memory[index + 1] = color;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }

    update_hardware_cursor();
}

void kprint(const char *str, char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        kprint_char(str[i], color);
    }
}

void int_to_string(int num, char *str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    int temp = num;
    while (temp > 0) {
        i++;
        temp /= 10;
    }
    str[i] = '\0';
    while (num > 0) {
        str[--i] = (num % 10) + '0';
        num /= 10;
    }
}