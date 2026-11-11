#ifndef TERMINAL_H
#define TERMINAL_H

#define TERM_MAX_ROWS 128
#define TERM_VISIBLE_ROWS 9
#define TERM_ROWS TERM_VISIBLE_ROWS
#define TERM_COLS 44
#define MAX_INPUT_LEN 40

typedef struct
{
    char buffer[TERM_MAX_ROWS][TERM_COLS];
    unsigned int buffer_color[TERM_MAX_ROWS];
    int total_lines;
    int scroll_offset;
    char input_buf[MAX_INPUT_LEN];
    int input_len;
    int cursor_row;
    int cursor_col;
} terminal_t;

void terminal_init(terminal_t *term);
void terminal_draw(terminal_t *term, int win_x, int win_y);
void terminal_handle_key(terminal_t *term, char c);
void terminal_write_line(terminal_t *term, const char *str);
void terminal_write_line_color(terminal_t *term, const char *str, unsigned int color);
void terminal_scroll_up(terminal_t *term, int lines);
void terminal_scroll_down(terminal_t *term, int lines);
int terminal_handle_click(terminal_t *term, int win_x, int win_y, int mouse_x, int mouse_y);

#endif