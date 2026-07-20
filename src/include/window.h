#ifndef WINDOW_H
#define WINDOW_H

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char *title;
    unsigned char is_dragging;
    int drag_offset_x;
    int drag_offset_y;
} window_t;

void window_init(window_t *win, int x, int y, int w, int h, const char *title);
void window_draw(window_t *win);
void window_handle_mouse(window_t *win, int mouse_x, int mouse_y, unsigned char left_btn);

#endif