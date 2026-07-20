#include "multiboot.h"
#include "tty.h"
#include "keyboard.h"
#include "shell.h"
#include "heap.h"
#include "vfs.h"
#include "task.h"
#include "vesa.h"
#include "mouse.h"

extern void init_gdt(void);
extern void init_idt(void);
extern void init_timer(void);
extern unsigned int end;
extern void init_pmm(struct multiboot_info* mb_info, unsigned int kernel_end_addr);

// System Call Dispatcher (int 0x80)
void isr_syscall_handler(unsigned int syscall_num, unsigned int arg1) {
    switch (syscall_num) {
        case 1:
            kprint((char *)arg1, 0x0B);
            break;
        case 2:
            kprint("\n[Kernel] Task User Mode telah selesai dan keluar.\n", 0x0C);
            break;
        case 3: {
            char c = 0;
            while ((c = get_last_key()) == 0) {
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

void render_desktop(mouse_state_t *mouse) {
    // 1. Gambar ke Back Buffer di RAM
    vesa_clear_screen(0x001F3F);

    // Window Utama
    vesa_draw_rect(300, 200, 400, 250, 0xFFFFFF); 
    vesa_draw_rect(300, 200, 400, 30, 0x0074D9);
    vesa_draw_rect(665, 205, 30, 20, 0xFF4136);

    vesa_draw_string(310, 207, "Lenovix OS - VESA Mode", 0xFFFFFF);
    vesa_draw_string(676, 207, "X", 0xFFFFFF);
    vesa_draw_string(320, 250, "Selamat Datang di Lenovix OS!", 0x000000);
    vesa_draw_string(320, 280, "Mode VESA VBE 1024x768 Active", 0x000000);

    // Taskbar & Button
    vesa_draw_rect(0, 728, 1024, 40, 0x111111);
    
    if (mouse->x >= 5 && mouse->x <= 85 && mouse->y >= 733 && mouse->y <= 763 && mouse->left_button) {
        vesa_draw_rect(5, 733, 80, 30, 0x2ECC71); // Highlight hijau terang saat diklik
    } else {
        vesa_draw_rect(5, 733, 80, 30, 0x2ECC40); 
    }
    vesa_draw_string(25, 740, "START", 0xFFFFFF);

    // Render Kursor Mouse
    vesa_draw_cursor(mouse->x, mouse->y);

    // 2. SALIN SAMPAI SELESAI KE LAYAR VRAM REAL!
    vesa_update();
}

void kernel_main(unsigned int magic, multiboot_info_t *mb_info) {
    (void)magic;

    init_vesa(mb_info);
    mouse_init();

    mouse_state_t *mouse = mouse_get_state();
    int last_x = -1, last_y = -1;
    unsigned char last_btn = 0;

    render_desktop(mouse);

    while(1) {
        mouse_handler();

        // Render ulang hanya jika mouse bergerak atau status klik berubah
        if (mouse->x != last_x || mouse->y != last_y || mouse->left_button != last_btn) {
            last_x = mouse->x;
            last_y = mouse->y;
            last_btn = mouse->left_button;
            render_desktop(mouse);
        }
    }
}