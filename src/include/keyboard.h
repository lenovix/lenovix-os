#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_handler_c(void);
char get_last_key(void);

extern volatile char last_ascii_char;

#endif