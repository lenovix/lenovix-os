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
#include "heap.h"
#include "vmm.h"
#include "vfs.h"
#include "ata.h"
#include "rtc.h"
#include "pci.h"
#include "rtl8139.h"
#include "speaker.h"
#include "acpi.h"
#include "bmp.h"
#include "sb16.h"
#include "net.h"
#include "tar.h"
#include "tcp.h"

extern void init_gdt(void);
extern void init_idt(void);
extern void init_timer(void);
extern unsigned int timer_ticks;
extern unsigned int end;
extern void init_pmm(struct multiboot_info *mb_info, unsigned int kernel_end_addr);
extern void init_heap(void);
extern void init_vmm(multiboot_info_t *mb_info);
extern void init_multitasking(void);

terminal_t main_term;

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
            terminal_write_line(&main_term, (char *)arg1);
        }
        else
        {
            kprint("[Kernel] ERROR: Invalid pointer in syscall 1\n", 0x0C);
            terminal_write_line(&main_term, "[Kernel] Invalid pointer in syscall 1");
        }
        break;
    case 2:
        kprint("\n[Kernel] Task User Mode telah selesai dan keluar.\n", 0x0C);
        terminal_write_line(&main_term, "[Kernel] User task exited.");
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
        terminal_write_line(&main_term, "[Kernel] Unknown syscall!");
        break;
    }
}

#define MAX_WINDOWS 7
static window_t windows[MAX_WINDOWS];
static int active_win = 0;
static start_menu_t start_menu;

void render_desktop(mouse_state_t *mouse)
{
    vesa_clear_screen(0x001F3F);
    bmp_draw_default_logo(512 - 64, 340 - 64);

    for (int i = 0; i < MAX_WINDOWS; i++)
    {
        if (i != active_win && windows[i].is_visible)
        {
            window_draw(&windows[i]);
        }
    }
    if (active_win >= 0 && active_win < MAX_WINDOWS && windows[active_win].is_visible)
    {
        window_draw(&windows[active_win]);
    }

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

    char time_buf[16];
    rtc_get_time_str(time_buf);
    vesa_draw_string(940, 740, time_buf, 0x00FF00);

    menu_draw(&start_menu, mouse->x, mouse->y);

    vesa_draw_cursor(mouse->x, mouse->y);

    vesa_update();
}

terminal_t active_terminal;
static multiboot_info_t local_mb_info;

void kernel_main(unsigned int magic, multiboot_info_t *mb_info)
{
    (void)magic;

    init_gdt();
    init_idt();

    terminal_init(&active_terminal);

    if (mb_info == NULL)
    {
        kprint("[FATAL] Multiboot info is NULL\n", 0x0C);
        return;
    }

    local_mb_info = *mb_info;

    init_pmm(&local_mb_info, (unsigned int)&end);
    init_vmm(&local_mb_info);
    init_heap();
    init_vfs();
    tar_init_embedded();
    ata_init();
    pci_init();
    rtl8139_init();
    net_init();
    tcp_init();
    sb16_init();
    rtc_init();
    acpi_init();
    init_timer();
    init_multitasking();

    init_vesa(&local_mb_info);
    mouse_init();

    __asm__ __volatile__("sti");

    terminal_init(&main_term);
    window_init(&windows[0], 280, 160, 420, 260, "Terminal", WIN_APP_TERMINAL);
    window_init(&windows[1], 120, 100, 360, 250, "File Explorer", WIN_APP_EXPLORER);
    window_init(&windows[2], 320, 180, 400, 250, "Task Manager", WIN_APP_SYSINFO);
    window_init(&windows[3], 200, 120, 380, 220, "Text Editor", WIN_APP_NOTEPAD);
    window_init(&windows[4], 240, 140, 420, 260, "Image Viewer", WIN_APP_VIEWER);
    window_init(&windows[5], 220, 120, 390, 270, "Retro Snake", WIN_APP_GAME);
    snake_init();
    window_init(&windows[6], 350, 150, 240, 260, "Calculator", WIN_APP_CALC);
    calc_init();
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
    speaker_play_chime();

    while (1)
    {
        net_poll();

        char key = get_last_key();
        if (key != 0)
        {
            if (active_win == 0 && windows[0].is_visible)
            {
                terminal_handle_key(&main_term, key);
                render_desktop(mouse);
            }
            else if (active_win == 3 && windows[3].is_visible)
            {
                notepad_handle_key(key);
                render_desktop(mouse);
            }
            else if (active_win == 5 && windows[5].is_visible)
            {
                snake_handle_key(key);
                render_desktop(mouse);
            }
            else if (active_win == 6 && windows[6].is_visible)
            {
                calc_handle_key(key);
                render_desktop(mouse);
            }
        }

        if (windows[5].is_visible)
        {
            static unsigned int last_snake_tick = 0;
            if (timer_ticks - last_snake_tick >= 10)
            {
                last_snake_tick = timer_ticks;
                snake_update();
                render_desktop(mouse);
            }
        }

        int in_start_btn = (mouse->x >= 5 && mouse->x <= 85 && mouse->y >= 733 && mouse->y <= 763);

        if (mouse->left_button && !prev_start_click)
        {
            speaker_click();
            if (in_start_btn)
            {
                start_menu.is_visible = !start_menu.is_visible;
            }
            else if (start_menu.is_visible)
            {
                int action = menu_handle_click(&start_menu, mouse->x, mouse->y);
                if (action >= 1 && action <= 7)
                {
                    windows[action - 1].is_visible = 1;
                    active_win = action - 1;
                    start_menu.is_visible = 0;
                }
                else if (action == 8)
                {
                    acpi_power_off();
                }
                else
                {
                    start_menu.is_visible = 0;
                }
            }
            else
            {
                int clicked_win = -1;
                if (active_win >= 0 && active_win < MAX_WINDOWS && windows[active_win].is_visible &&
                    mouse->x >= windows[active_win].x && mouse->x <= windows[active_win].x + windows[active_win].width &&
                    mouse->y >= windows[active_win].y && mouse->y <= windows[active_win].y + windows[active_win].height)
                {
                    clicked_win = active_win;
                }
                else
                {
                    for (int w = MAX_WINDOWS - 1; w >= 0; w--)
                    {
                        if (windows[w].is_visible &&
                            mouse->x >= windows[w].x && mouse->x <= windows[w].x + windows[w].width &&
                            mouse->y >= windows[w].y && mouse->y <= windows[w].y + windows[w].height)
                        {
                            clicked_win = w;
                            break;
                        }
                    }
                }

                if (clicked_win != -1)
                {
                    active_win = clicked_win;
                    if (clicked_win == 1)
                    {
                        char fname[32];
                        char fcontent[512];
                        if (explorer_handle_click(&windows[1], mouse->x, mouse->y, fname, fcontent) >= 0)
                        {
                            notepad_set_text(fname, fcontent);
                            windows[3].is_visible = 1;
                            active_win = 3;
                        }
                    }
                    window_handle_mouse(&windows[active_win], mouse->x, mouse->y, mouse->left_button);
                }
            }
        }
        else if (mouse->left_button && active_win >= 0 && active_win < MAX_WINDOWS && windows[active_win].is_dragging)
        {
            window_handle_mouse(&windows[active_win], mouse->x, mouse->y, mouse->left_button);
        }
        else if (!mouse->left_button && active_win >= 0 && active_win < MAX_WINDOWS && windows[active_win].is_dragging)
        {
            windows[active_win].is_dragging = 0;
        }

        prev_start_click = mouse->left_button;

        int any_dragging = (active_win >= 0 && active_win < MAX_WINDOWS && windows[active_win].is_dragging);
        if (mouse->x != last_x || mouse->y != last_y || mouse->left_button != last_btn || any_dragging)
        {
            last_x = mouse->x;
            last_y = mouse->y;
            last_btn = mouse->left_button;
            render_desktop(mouse);
        }
    }
}