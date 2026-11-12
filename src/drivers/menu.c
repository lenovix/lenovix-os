#include "menu.h"
#include "vesa.h"

void menu_init(start_menu_t *menu)
{
    menu->x = 5;
    menu->width = 220;
    menu->height = 425;
    menu->y = 728 - menu->height - 5;
    menu->is_visible = 0;
}

void menu_draw(start_menu_t *menu, int mouse_x, int mouse_y)
{
    if (!menu->is_visible)
        return;

    vesa_draw_rect(menu->x - 2, menu->y - 2, menu->width + 4, menu->height + 4, 0x000000);
    vesa_draw_rect(menu->x, menu->y, menu->width, menu->height, 0x1E1E1E);

    vesa_draw_rect(menu->x, menu->y, menu->width, 30, 0x0074D9);
    vesa_draw_string(menu->x + 10, menu->y + 7, "Lenovix OS Menu", 0xFFFFFF);

    // Item 1: Terminal
    int y1 = menu->y + 35;
    int h1 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y1 && mouse_y <= y1 + 30);
    vesa_draw_rect(menu->x + 5, y1, menu->width - 10, 30, h1 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y1 + 7, ">_ Terminal", 0xFFFFFF);

    // Item 2: File Explorer
    int y2 = menu->y + 70;
    int h2 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y2 && mouse_y <= y2 + 30);
    vesa_draw_rect(menu->x + 5, y2, menu->width - 10, 30, h2 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y2 + 7, "[F] File Explorer", 0xFFFFFF);

    // Item 3: Task Manager
    int y3 = menu->y + 105;
    int h3 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y3 && mouse_y <= y3 + 30);
    vesa_draw_rect(menu->x + 5, y3, menu->width - 10, 30, h3 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y3 + 7, "[%] Task Manager", 0xFFFFFF);

    // Item 4: Text Editor
    int y4 = menu->y + 140;
    int h4 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y4 && mouse_y <= y4 + 30);
    vesa_draw_rect(menu->x + 5, y4, menu->width - 10, 30, h4 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y4 + 7, "[=] Text Editor", 0xFFFFFF);

    // Item 5: Image Viewer
    int y5 = menu->y + 175;
    int h5 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y5 && mouse_y <= y5 + 30);
    vesa_draw_rect(menu->x + 5, y5, menu->width - 10, 30, h5 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y5 + 7, "[*] Image Viewer", 0xFFFFFF);

    // Item 6: Snake Game
    int y6 = menu->y + 210;
    int h6 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y6 && mouse_y <= y6 + 30);
    vesa_draw_rect(menu->x + 5, y6, menu->width - 10, 30, h6 ? 0x2ECC40 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y6 + 7, "[S] Snake Game", 0xFFFFFF);

    // Item 7: Calculator
    int y7 = menu->y + 245;
    int h7 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y7 && mouse_y <= y7 + 30);
    vesa_draw_rect(menu->x + 5, y7, menu->width - 10, 30, h7 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y7 + 7, "[+] Calculator", 0xFFFFFF);

    // Item 8: Paint Studio
    int y8 = menu->y + 280;
    int h8 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y8 && mouse_y <= y8 + 30);
    vesa_draw_rect(menu->x + 5, y8, menu->width - 10, 30, h8 ? 0x00D2FF : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y8 + 7, "[P] Paint Studio", 0xFFFFFF);

    // Item 9: Media Player
    int y9 = menu->y + 315;
    int h9 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y9 && mouse_y <= y9 + 30);
    vesa_draw_rect(menu->x + 5, y9, menu->width - 10, 30, h9 ? 0x2ECC40 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y9 + 7, "[M] Media Player", 0xFFFFFF);

    // Item 10: Web Browser
    int y10 = menu->y + 350;
    int h10 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y10 && mouse_y <= y10 + 30);
    vesa_draw_rect(menu->x + 5, y10, menu->width - 10, 30, h10 ? 0x0074D9 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y10 + 7, "[W] Web Browser", 0xFFFFFF);

    // Item 11: Shutdown
    int y11 = menu->y + 385;
    int h11 = (mouse_x >= menu->x && mouse_x <= menu->x + menu->width && mouse_y >= y11 && mouse_y <= y11 + 30);
    vesa_draw_rect(menu->x + 5, y11, menu->width - 10, 30, h11 ? 0xFF4136 : 0x2A2A2A);
    vesa_draw_string(menu->x + 15, y11 + 7, "[O] Shutdown", 0xFFFFFF);
}

int menu_handle_click(start_menu_t *menu, int mouse_x, int mouse_y)
{
    if (!menu->is_visible)
        return 0;

    if (mouse_x < menu->x || mouse_x > menu->x + menu->width ||
        mouse_y < menu->y || mouse_y > menu->y + menu->height)
    {
        menu->is_visible = 0;
        return 0;
    }

    if (mouse_y >= menu->y + 35 && mouse_y <= menu->y + 65)
    {
        menu->is_visible = 0;
        return 1; // Terminal
    }
    if (mouse_y >= menu->y + 70 && mouse_y <= menu->y + 100)
    {
        menu->is_visible = 0;
        return 2; // File Explorer
    }
    if (mouse_y >= menu->y + 105 && mouse_y <= menu->y + 135)
    {
        menu->is_visible = 0;
        return 3; // Task Manager
    }
    if (mouse_y >= menu->y + 140 && mouse_y <= menu->y + 170)
    {
        menu->is_visible = 0;
        return 4; // Text Editor
    }
    if (mouse_y >= menu->y + 175 && mouse_y <= menu->y + 205)
    {
        menu->is_visible = 0;
        return 5; // Image Viewer
    }
    if (mouse_y >= menu->y + 210 && mouse_y <= menu->y + 240)
    {
        menu->is_visible = 0;
        return 6; // Snake Game
    }
    if (mouse_y >= menu->y + 245 && mouse_y <= menu->y + 275)
    {
        menu->is_visible = 0;
        return 7; // Calculator
    }
    if (mouse_y >= menu->y + 280 && mouse_y <= menu->y + 310)
    {
        menu->is_visible = 0;
        return 8; // Paint Studio
    }
    if (mouse_y >= menu->y + 315 && mouse_y <= menu->y + 345)
    {
        menu->is_visible = 0;
        return 9; // Media Player
    }
    if (mouse_y >= menu->y + 350 && mouse_y <= menu->y + 380)
    {
        menu->is_visible = 0;
        return 10; // Web Browser
    }
    if (mouse_y >= menu->y + 385 && mouse_y <= menu->y + 415)
    {
        menu->is_visible = 0;
        return 11; // Shutdown
    }

    return 0;
}
