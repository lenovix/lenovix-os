#ifndef MENU_H
#define MENU_H

typedef struct {
    int x;
    int y;
    int width;
    int height;
    unsigned char is_visible;
} start_menu_t;

void menu_init(start_menu_t *menu);
void menu_draw(start_menu_t *menu, int mouse_x, int mouse_y);
int menu_handle_click(start_menu_t *menu, int mouse_x, int mouse_y);

#endif