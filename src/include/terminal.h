#ifndef TERMINAL_H
#define TERMINAL_H

#define TERM_ROWS 10
#define TERM_COLS 42
#define MAX_INPUT_LEN 40

typedef struct {
    char buffer[TERM_ROWS][TERM_COLS];
    char input_buf[MAX_INPUT_LEN];
    int input_len;
    int cursor_row;
    int cursor_col;
} terminal_t;

void terminal_init(terminal_t *term);
void terminal_draw(terminal_t *term, int win_x, int win_y);
void terminal_handle_key(terminal_t *term, char c);
void terminal_write_line(terminal_t *term, const char *str);

#endif