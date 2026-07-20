#include "multiboot.h"
#include "tty.h"
#include "keyboard.h"
#include "shell.h"
#include "heap.h"
#include "vfs.h"
#include "task.h"

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

// Entry Point Kernel
void kernel_main(struct multiboot_info* mb_info) {
    init_gdt();
    clear_screen();

    kprint("Selamat Datang di Lenovix OS!\n", 0x0A);
    kprint("-----------------------------\n", 0x07);
    
    kprint("Inisialisasi GDT... ", 0x0F); kprint("[ OK ]\n", 0x0A);
    
    kprint("Mengaktifkan IDT & Interrupts... ", 0x0F);
    init_idt();
    kprint("[ OK ]\n", 0x0A);
    
    init_timer();
    
    kprint("Mengaktifkan PMM... ", 0x0F);
    init_pmm(mb_info, (unsigned int)&end);
    kprint("[ OK ]\n", 0x0A);

    kprint("Inisialisasi Dynamic Heap... ", 0x0F);
    init_heap();
    kprint("[ OK ]\n", 0x0A);

    kprint("Inisialisasi VFS... ", 0x0F);
    init_vfs();
    kprint("[ OK ]\n", 0x0A);

    kprint("Inisialisasi Multitasking... ", 0x0F);
    init_multitasking();
    kprint("[ OK ]\n", 0x0A);

    kprint("Silakan ketik perintah Anda:\n\n> ", 0x0E);

    while(1) {
        asm volatile("hlt");
    }
}