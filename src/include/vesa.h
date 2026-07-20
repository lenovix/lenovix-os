#ifndef VESA_H
#define VESA_H

#include <stddef.h>
#include "multiboot.h"

// Fungsi Driver VESA
void init_vesa(multiboot_info_t *mb_info);
void vesa_put_pixel(int x, int y, unsigned int color);
void vesa_clear_screen(unsigned int color);
void vesa_draw_rect(int x, int y, int width, int height, unsigned int color);
void vesa_draw_char(int x, int y, char c, unsigned int color, unsigned int bg_color);
void vesa_draw_string(int x, int y, const char *str, unsigned int color);
void vesa_update(void);

#endif