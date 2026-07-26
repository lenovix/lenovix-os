#include "multiboot.h"
#include "tty.h"
#include "keyboard.h"
#include "shell.h"
#include "heap.h"
#include "vfs.h"
#include "task.h"
#include "vesa.h"
#include "mouse.h"
#include "window.h"
#include "menu.h"
#include "terminal.h"

extern void init_gdt(void);
extern void init_idt(void);
extern void init_timer(void);
extern unsigned int end;
extern void init_pmm(struct multiboot_info *mb_info, unsigned int kernel_end_addr);

// System Call Dispatcher (int 0x80)
void isr_syscall_handler(unsigned int syscall_num, unsigned int arg1)
{
    switch (syscall_num)
    {
    case 1:
        kprint((char *)arg1, 0x0B);
        break;
    case 2:
        kprint("\n[Kernel] Task User Mode telah selesai dan keluar.\n", 0x0C);
        break;
    case 3:
    {
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

// void kernel_main(struct multiboot_info* mb_info) {
//     init_gdt();
//     clear_screen();

//     kprint("Selamat Datang di Lenovix OS!\n", 0x0A);
//     kprint("-----------------------------\n", 0x07);

//     kprint("Inisialisasi GDT... ", 0x0F); kprint("[ OK ]\n", 0x0A);

//     kprint("Mengaktifkan IDT & Interrupts... ", 0x0F);
//     init_idt();
//     kprint("[ OK ]\n", 0x0A);

//     init_timer();

//     kprint("Mengaktifkan PMM... ", 0x0F);
//     init_pmm(mb_info, (unsigned int)&end);
//     kprint("[ OK ]\n", 0x0A);

//     kprint("Inisialisasi Dynamic Heap... ", 0x0F);
//     init_heap();
//     kprint("[ OK ]\n", 0x0A);

//     kprint("Inisialisasi VFS... ", 0x0F);
//     init_vfs();
//     kprint("[ OK ]\n", 0x0A);

//     kprint("Inisialisasi Multitasking... ", 0x0F);
//     init_multitasking();
//     kprint("[ OK ]\n", 0x0A);

//     kprint("Inisialisasi Driver VESA... ", 0x0F);
//     init_vesa(mb_info);
//     kprint("[ OK ]\n", 0x0A);

//     kprint("Silakan ketik perintah Anda:\n\n> ", 0x0E);

//     while(1) {
//         asm volatile("hlt");
//     }
// }

static window_t main_win;
static start_menu_t start_menu;
terminal_t main_term;

static inline void outb(unsigned short port, unsigned char val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(unsigned short port, unsigned short val)
{
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

void render_desktop(mouse_state_t *mouse) {
    // 1. Background Desktop
    vesa_clear_screen(0x001F3F);

    // 2. Draw Window Utama (Termasuk Terminal)
    window_draw(&main_win);

    // 3. Taskbar & Start Button
    vesa_draw_rect(0, 728, 1024, 40, 0x111111);

    int start_btn_hover = (mouse->x >= 5 && mouse->x <= 85 && mouse->y >= 733 && mouse->y <= 763);
    if ((start_btn_hover && mouse->left_button) || start_menu.is_visible) {
        vesa_draw_rect(5, 733, 80, 30, 0x2ECC71);
    } else {
        vesa_draw_rect(5, 733, 80, 30, 0x2ECC40);
    }
    vesa_draw_string(25, 740, "START", 0xFFFFFF);

    // 4. Render Start Menu
    menu_draw(&start_menu, mouse->x, mouse->y);

    // 5. Cursor Mouse
    vesa_draw_cursor(mouse->x, mouse->y);

    // 6. Flip Buffer
    vesa_update();
}

terminal_t active_terminal;

void kernel_main(unsigned int magic, multiboot_info_t *mb_info) {
    terminal_init(&active_terminal);

    (void)magic;

    init_vesa(mb_info);
    mouse_init();

    // Inisialisasi Window & Terminal Engine
    window_init(&main_win, 300, 200, 400, 250, "Lenovix Terminal");
    terminal_init(&main_term);
    menu_init(&start_menu);

    mouse_state_t *mouse = mouse_get_state();
    int last_x = -1, last_y = -1;
    unsigned char last_btn = 0;
    unsigned char prev_start_click = 0;

    render_desktop(mouse);

    while(1) {
        mouse_handler();

        // 1. Tangkap Input Keyboard untuk Terminal
        char key = get_last_key(); // Fungsi dari driver keyboard.c
        if (key != 0 && main_win.is_visible) {
            terminal_handle_key(&main_term, key);
            render_desktop(mouse); // Re-render saat ada karakter diketik
        }

        // 2. Logika Mouse Event
        int in_start_btn = (mouse->x >= 5 && mouse->x <= 85 && mouse->y >= 733 && mouse->y <= 763);
        
        if (mouse->left_button && !prev_start_click) {
            if (in_start_btn) {
                start_menu.is_visible = !start_menu.is_visible;
            } else if (start_menu.is_visible) {
                int action = menu_handle_click(&start_menu, mouse->x, mouse->y);
                if (action == 1) { // About OS / Open Window
                    main_win.is_visible = 1;
                    start_menu.is_visible = 0;
                } else if (action == 2) { // Open Terminal Window
                    main_win.is_visible = 1;
                    start_menu.is_visible = 0;
                } else if (action == 3) { // Shutdown QEMU
                    outw(0x604, 0x2000);
                } else {
                    start_menu.is_visible = 0;
                    window_handle_mouse(&main_win, mouse->x, mouse->y, mouse->left_button);
                }
            } else {
                window_handle_mouse(&main_win, mouse->x, mouse->y, mouse->left_button);
            }
        } else if (mouse->left_button && main_win.is_dragging) {
            window_handle_mouse(&main_win, mouse->x, mouse->y, mouse->left_button);
        } else if (!mouse->left_button && main_win.is_dragging) {
            main_win.is_dragging = 0;
        }

        prev_start_click = mouse->left_button;

        // 3. Render ulang jika mouse bergerak atau di-drag
        if (mouse->x != last_x || mouse->y != last_y || mouse->left_button != last_btn || main_win.is_dragging) {
            last_x = mouse->x;
            last_y = mouse->y;
            last_btn = mouse->left_button;
            render_desktop(mouse);
        }
    }
}