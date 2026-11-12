#ifndef BROWSER_H
#define BROWSER_H

#include "window.h"

void browser_init(void);
void browser_draw(window_t *win);
int browser_handle_click(window_t *win, int mouse_x, int mouse_y);
void browser_handle_key(char c);
void browser_navigate(const char *url);
void window_show_browser(void);

#endif
