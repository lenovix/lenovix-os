#include "multiboot.h"
#include "tty.h"
#include "keyboard.h"
#include "shell.h"
#include "heap.h"
#include "vfs.h"
#include "task.h"
#include "vesa.h"

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

void kernel_main(unsigned int magic, multiboot_info_t *mb_info) {
    // Menghindari compiler warning unused variable 'magic'
    (void)magic;

    // Inisialisasi driver VESA VBE
    init_vesa(mb_info);

    // 1. Cat seluruh layar dengan warna Biru Tua (0x001F3F)
    vesa_clear_screen(0x001F3F);

    // 2. Gambar Jendela GUI Sederhana di tengah layar
    // Jendela Putih (X: 300, Y: 200, Width: 400, Height: 250)
    vesa_draw_rect(300, 200, 400, 250, 0xFFFFFF); 

    // Header Jendela Warna Biru Cerah (X: 300, Y: 200, Width: 400, Height: 30)
    vesa_draw_rect(300, 200, 400, 30, 0x0074D9);

    // Tombol Close Merah (X: 665, Y: 205, Width: 30, Height: 20)
    vesa_draw_rect(665, 205, 30, 20, 0xFF4136);

    // 3. Render Teks pada Jendela GUI
    // Judul Jendela di Header (Putih)
    vesa_draw_string(310, 207, "Lenovix OS - VESA Mode", 0xFFFFFF);

    // Karakter 'X' pada Tombol Close Merah (Putih)
    vesa_draw_string(676, 207, "X", 0xFFFFFF);

    // Isi Pesan di Dalam Jendela (Hitam)
    vesa_draw_string(320, 250, "Selamat Datang di Lenovix OS!", 0x000000);
    vesa_draw_string(320, 280, "Mode VESA VBE 1024x768 Active", 0x000000);

    // 4. Taskbar Sederhana di Bagian Bawah Layar
    vesa_draw_rect(0, 728, 1024, 40, 0x111111); // Taskbar Hitam
    vesa_draw_rect(5, 733, 80, 30, 0x2ECC40);   // Tombol Start Hijau

    // Teks 'START' di dalam Tombol Start (Putih)
    vesa_draw_string(25, 740, "START", 0xFFFFFF);

    // Halt CPU agar tampilan tidak hilang
    while(1) {
        asm volatile("hlt");
    }
}