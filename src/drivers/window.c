#include "window.h"
#include "vesa.h"

void window_init(window_t *win, int x, int y, int w, int h, const char *title) {
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->title = title;
    win->is_dragging = 0;
    win->drag_offset_x = 0;
    win->drag_offset_y = 0;
}

void window_draw(window_t *win) {
    // 1. Body Window (Putih)
    vesa_draw_rect(win->x, win->y, win->width, win->height, 0xFFFFFF);

    // 2. Title Bar (Biru)
    vesa_draw_rect(win->x, win->y, win->width, 30, 0x0074D9);

    // 3. Tombol Close 'X' (Merah)
    vesa_draw_rect(win->x + win->width - 35, win->y + 5, 30, 20, 0xFF4136);
    vesa_draw_string(win->x + win->width - 24, win->y + 7, "X", 0xFFFFFF);

    // 4. Judul Window
    vesa_draw_string(win->x + 10, win->y + 7, win->title, 0xFFFFFF);

    // 5. Isi Window
    vesa_draw_string(win->x + 20, win->y + 50, "Selamat Datang di Lenovix OS!", 0x000000);
    vesa_draw_string(win->x + 20, win->y + 80, "Coba geser window ini!", 0x000000);
}

void window_handle_mouse(window_t *win, int mouse_x, int mouse_y, unsigned char left_btn) {
    int titlebar_h = 30;

    if (left_btn) {
        if (!win->is_dragging) {
            // Cek apakah klik pertama terjadi di area Title Bar (selain tombol X)
            int in_titlebar = (mouse_x >= win->x && mouse_x <= (win->x + win->width - 40) &&
                               mouse_y >= win->y && mouse_y <= (win->y + titlebar_h));

            if (in_titlebar) {
                win->is_dragging = 1;
                win->drag_offset_x = mouse_x - win->x;
                win->drag_offset_y = mouse_y - win->y;
            }
        } else {
            // Jika sedang di-drag, perbarui posisi window sesuai pergerakan mouse
            win->x = mouse_x - win->drag_offset_x;
            win->y = mouse_y - win->drag_offset_y;

            // Clamp agar window tidak hilang tergeser melebihi batas layar
            if (win->x < 0) win->x = 0;
            if (win->y < 0) win->y = 0;
            if (win->x + win->width > 1024) win->x = 1024 - win->width;
            if (win->y + win->height > 728) win->y = 728 - win->height; // Batas atas taskbar
        }
    } else {
        // Lepas drag saat tombol mouse dilepas
        win->is_dragging = 0;
    }
}