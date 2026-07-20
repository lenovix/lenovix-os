#include "vesa.h"

// Array font 8x16 sederhana untuk beberapa karakter / karakter standar
// (Sebagai contoh dasar rendering bitmask font)
void vesa_draw_char(int x, int y, char c, unsigned int color, unsigned int bg_color) {
    // Implementasi simpel menggunakan bitmask
    // Jika bit = 1, vesa_put_pixel(x, y, color)
    // Jika bit = 0 & bg_color != 0, vesa_put_pixel(x, y, bg_color)
}

void vesa_draw_string(int x, int y, const char *str, unsigned int color) {
    int cur_x = x;
    while (*str) {
        // Panggil draw char per karakter
        // vesa_draw_char(cur_x, y, *str, color, 0);
        cur_x += 8; // Geser 8 pixel ke kanan untuk karakter berikutnya
        str++;
    }
}