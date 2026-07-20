#ifndef TTY_H
#define TTY_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void clear_screen(void);
void kprint_char(char c, char color);
void kprint(const char *str, char color);
void update_hardware_cursor(void);
void int_to_string(int num, char *str);

#endif