#ifndef PLAYER_H
#define PLAYER_H

#include "window.h"

void player_init(void);
void player_draw(window_t *win);
int player_handle_click(window_t *win, int mouse_x, int mouse_y);
void player_handle_key(char c);

#endif
