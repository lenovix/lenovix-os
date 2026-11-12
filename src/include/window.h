#ifndef WINDOW_H
#define WINDOW_H

#define WIN_APP_TERMINAL 0
#define WIN_APP_EXPLORER 1
#define WIN_APP_SYSINFO 2
#define WIN_APP_NOTEPAD 3
#define WIN_APP_VIEWER 4
#define WIN_APP_GAME 5
#define WIN_APP_CALC 6
#define WIN_APP_PAINT 7
#define WIN_APP_PLAYER 8

typedef struct
{
    int x;
    int y;
    int width;
    int height;
    const char *title;
    int app_type;
    unsigned char is_dragging;
    unsigned char is_visible;
    int drag_offset_x;
    int drag_offset_y;
} window_t;

void window_init(window_t *win, int x, int y, int w, int h, const char *title, int app_type);
void window_draw(window_t *win);
void window_handle_mouse(window_t *win, int mouse_x, int mouse_y, unsigned char left_btn);

void notepad_handle_key(char c);
void notepad_set_text(const char *name, const char *text);
const char *notepad_get_text(void);
int explorer_handle_click(window_t *win, int mouse_x, int mouse_y, char *out_name, char *out_content);

void snake_init(void);
void snake_update(void);
void snake_handle_key(char c);

void calc_init(void);
void calc_handle_key(char c);
int calc_handle_click(window_t *win, int mouse_x, int mouse_y);

void paint_init(void);
int paint_handle_mouse(window_t *win, int mouse_x, int mouse_y, unsigned char left_btn);

void player_init(void);
void player_draw(window_t *win);
int player_handle_click(window_t *win, int mouse_x, int mouse_y);
void player_handle_key(char c);
void window_show_player(void);

#endif