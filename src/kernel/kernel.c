#include "multiboot.h"
#include "tty.h"
#include "keyboard.h"
#include "task.h"
#include "vesa.h"
#include "mouse.h"
#include "window.h"
#include "menu.h"
#include "terminal.h"
#include "io_util.h"

extern void init_gdt(void);
extern void init_idt(void);
extern void init_timer(void);
extern unsigned int end;
extern void init_pmm(struct multiboot_info *mb_info, unsigned int kernel_end_addr);

#define KERNEL_SPACE_END 0x01000000
static int is_valid_user_pointer(unsigned int ptr)
{
    return (ptr > 0 && ptr < KERNEL_SPACE_END);
}

void isr_syscall_handler(unsigned int syscall_num, unsigned int arg1)
{
    switch (syscall_num)
    {
    case 1:
        if (is_valid_user_pointer(arg1))
        {
            kprint((char *)arg1, 0x0B);
        }
        else
        {
            kprint("[Kernel] ERROR: Invalid pointer in syscall 1\n", 0x0C);
        }
        break;
    case 2:
        kprint("\n[Kernel] Task User Mode telah selesai dan keluar.\n", 0x0C);
        break;
    case 3:
    {
        if (!is_valid_user_pointer(arg1))
        {
            kprint("[Kernel] ERROR: Invalid pointer in syscall 3\n", 0x0C);
            break;
        }
        char c = 0;
        while ((c = get_last_key()) == 0)
        {
            task_yield();
        }
        char *out_char = (char *)arg1;
        *out_char = c;
        break;
    }
    default:
        kprint("Syscall tidak dikenal!\n", 0x0C);
        break;
    }
}

static window_t main_win;
static start_menu_t start_menu;
terminal_t main_term;

void render_desktop(mouse_state_t *mouse)
{
    vesa_clear_screen(0x001F3F);

    window_draw(&main_win);

    vesa_draw_rect(0, 728, 1024, 40, 0x111111);

    int start_btn_hover = (mouse->x >= 5 && mouse->x <= 85 && mouse->y >= 733 && mouse->y <= 763);
    if ((start_btn_hover && mouse->left_button) || start_menu.is_visible)
    {
        vesa_draw_rect(5, 733, 80, 30, 0x2ECC71);
    }
    else
    {
        vesa_draw_rect(5, 733, 80, 30, 0x2ECC40);
    }
    vesa_draw_string(25, 740, "START", 0xFFFFFF);

    menu_draw(&start_menu, mouse->x, mouse->y);

    vesa_draw_cursor(mouse->x, mouse->y);

    vesa_update();
}

terminal_t active_terminal;

void kernel_main(unsigned int magic, multiboot_info_t *mb_info)
{
    terminal_init(&active_terminal);

    (void)magic;

    if (mb_info == NULL)
    {
        kprint("[FATAL] Multiboot info is NULL\n", 0x0C);
        return;
    }

    init_vesa(mb_info);

    mouse_init();

    window_init(&main_win, 300, 200, 400, 250, "Lenovix Terminal");
    terminal_init(&main_term);
    menu_init(&start_menu);

    mouse_state_t *mouse = mouse_get_state();
    if (mouse == NULL)
    {
        kprint("[ERROR] Failed to get mouse state\n", 0x0C);
        return;
    }

    int last_x = -1, last_y = -1;
    unsigned char last_btn = 0;
    unsigned char prev_start_click = 0;

    render_desktop(mouse);

    while (1)
    {
        mouse_handler();

        char key = get_last_key();
        if (key != 0 && main_win.is_visible)
        {
            terminal_handle_key(&main_term, key);
            render_desktop(mouse);
        }

        int in_start_btn = (mouse->x >= 5 && mouse->x <= 85 && mouse->y >= 733 && mouse->y <= 763);

        if (mouse->left_button && !prev_start_click)
        {
            if (in_start_btn)
            {
                start_menu.is_visible = !start_menu.is_visible;
            }
            else if (start_menu.is_visible)
            {
                int action = menu_handle_click(&start_menu, mouse->x, mouse->y);
                if (action == 1)
                {
                    main_win.is_visible = 1;
                    start_menu.is_visible = 0;
                }
                else if (action == 2)
                {
                    main_win.is_visible = 1;
                    start_menu.is_visible = 0;
                }
                else if (action == 3)
                {
                    outw(0x604, 0x2000);
                }
                else
                {
                    start_menu.is_visible = 0;
                    window_handle_mouse(&main_win, mouse->x, mouse->y, mouse->left_button);
                }
            }
            else
            {
                window_handle_mouse(&main_win, mouse->x, mouse->y, mouse->left_button);
            }
        }
        else if (mouse->left_button && main_win.is_dragging)
        {
            window_handle_mouse(&main_win, mouse->x, mouse->y, mouse->left_button);
        }
        else if (!mouse->left_button && main_win.is_dragging)
        {
            main_win.is_dragging = 0;
        }

        prev_start_click = mouse->left_button;

        if (mouse->x != last_x || mouse->y != last_y || mouse->left_button != last_btn || main_win.is_dragging)
        {
            last_x = mouse->x;
            last_y = mouse->y;
            last_btn = mouse->left_button;
            render_desktop(mouse);
        }
    }
}