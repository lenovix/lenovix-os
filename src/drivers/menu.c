#include "menu.h"
#include "vesa.h"

void menu_init(start_menu_t *menu) {
    menu->x = 5;
    menu->width = 200;
    menu->height = 150;
    menu->y = 728 - menu->height - 5; // Posisikan tepat di atas taskbar (y = 573)
    menu->is_visible = 0;
}

void menu_draw(start_menu_t *menu, int mouse_x, int mouse_y) {
    if (!menu->is_visible) return;

    // 1. Shadow / Border Menu
    vesa_draw_rect(menu->x - 2, menu->y - 2, menu->width + 4, menu->height + 4, 0x000000);

    // 2. Background Utama Menu (Abu-abu Gelap Modern)
    vesa_draw_rect(menu->x, menu->y, menu->width, menu->height, 0x1E1E1E);

    // 3. Header Menu
    vesa_draw_rect(menu->x, menu->y, menu->width, 30, 0x0074D9);
    vesa_draw_string(menu->x + 10, menu->y + 7, "Lenovix OS Menu", 0xFFFFFF);

    // 4. Opsi Menu
    // Item 1: About OS
    int item1_y = menu->y + 35;
    int hover1 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width &&
                  mouse_y >= item1_y && mouse_y <= item1_y + 30);
    vesa_draw_rect(menu->x + 5, item1_y, menu->width - 10, 30, hover1 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, item1_y + 7, "i  About OS", 0xFFFFFF);

    // Item 2: Terminal
    int item2_y = menu->y + 70;
    int hover2 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width &&
                  mouse_y >= item2_y && mouse_y <= item2_y + 30);
    vesa_draw_rect(menu->x + 5, item2_y, menu->width - 10, 30, hover2 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, item2_y + 7, ">_ Terminal", 0xFFFFFF);

    // Item 3: Shutdown
    int item3_y = menu->y + 105;
    int hover3 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width &&
                  mouse_y >= item3_y && mouse_y <= item3_y + 30);
    vesa_draw_rect(menu->x + 5, item3_y, menu->width - 10, 30, hover3 ? 0xFF4136 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, item3_y + 7, "O  Shutdown", 0xFFFFFF);
}

// Return ID opsi yang diklik (1: About, 2: Terminal, 3: Shutdown, 0: Luar Menu)
int menu_handle_click(start_menu_t *menu, int mouse_x, int mouse_y) {
    if (!menu->is_visible) return 0;

    // Jika klik di luar area menu, tutup menu
    if (mouse_x < menu->x || mouse_x > menu->x + menu->width ||
        mouse_y < menu->y || mouse_y > menu->y + menu->height) {
        menu->is_visible = 0;
        return 0;
    }

    // Cek item 1: About OS
    if (mouse_y >= menu->y + 35 && mouse_y <= menu->y + 65) {
        menu->is_visible = 0;
        return 1;
    }
    // Cek item 2: Terminal
    if (mouse_y >= menu->y + 70 && mouse_y <= menu->y + 100) {
        menu->is_visible = 0;
        return 2;
    }
    // Cek item 3: Shutdown
    if (mouse_y >= menu->y + 105 && mouse_y <= menu->y + 135) {
        menu->is_visible = 0;
        return 3;
    }

    return 0;
}