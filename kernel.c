#include "multiboot.h"

int cursor_x = 0;
int cursor_y = 0;
const int VGA_WIDTH = 80;
const int VGA_HEIGHT = 25;
char *video_memory = (char *) 0xB8000;

char command_buffer[256];
int command_index = 0;

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);
extern void shutdown_qemu(void);
extern void init_idt(void); // Ambil fungsi inisialisasi IDT
extern void init_gdt(void);
extern void init_timer(void);
extern void sleep(unsigned int ticks);
extern unsigned int timer_ticks;

// Variabel simbol eksternal dari Linker Script untuk mengetahui titik akhir kernel di RAM
extern unsigned int end; 
extern void init_pmm(struct multiboot_info* mb_info, unsigned int kernel_end_addr);

int string_compare(const char *str1, const char *str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) return 0;
        i++;
    }
    return (str1[i] == '\0' && str2[i] == '\0');
}

void update_hardware_cursor(void) {
    unsigned short position = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = 0x07;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_hardware_cursor();
}

void scroll(void) {
    // Geser baris 1-24 naik ke baris 0-23
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            int target_index = ((y - 1) * VGA_WIDTH + x) * 2;
            int source_index = (y * VGA_WIDTH + x) * 2;
            video_memory[target_index] = video_memory[source_index];
            video_memory[target_index + 1] = video_memory[source_index + 1];
        }
    }

    // Kosongkan baris paling bawah (baris 24)
    for (int x = 0; x < VGA_WIDTH; x++) {
        int index = ((VGA_HEIGHT - 1) * VGA_WIDTH + x) * 2;
        video_memory[index] = ' ';
        video_memory[index + 1] = 0x07;
    }

    // Kembalikan kursor ke baris paling bawah
    cursor_y = VGA_HEIGHT - 1;
}

void kprint_char(char c, char color) {
    if (c == '\b') {
        if (cursor_x > 2) {
            cursor_x--;
            int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
            video_memory[index] = ' ';
            video_memory[index + 1] = color;
        }
        update_hardware_cursor();
        return;
    }

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
        video_memory[index] = c;
        video_memory[index + 1] = color;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }

    update_hardware_cursor();
}

void kprint(const char *str, char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        kprint_char(str[i], color);
    }
}

void int_to_string(int num, char *str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    int temp = num;
    while (temp > 0) {
        i++;
        temp /= 10;
    }
    str[i] = '\0';
    while (num > 0) {
        str[--i] = (num % 10) + '0';
        num /= 10;
    }
}

void process_command(void) {
    kprint("\n", 0x07);
    command_buffer[command_index] = '\0';

    if (command_index == 0) {
        kprint("> ", 0x0F);
        return;
    }

    if (string_compare(command_buffer, "help")) {
        kprint("Perintah Lenovix OS yang tersedia:\n", 0x0E);
        kprint("  help     - Menampilkan daftar perintah ini\n", 0x0F);
        kprint("  about    - Informasi mengenai sistem operasi ini\n", 0x0F);
        kprint("  uptime   - Menampilkan durasi aktif sistem operasi\n", 0x0F);
        kprint("  clear    - Membersihkan layar shell\n", 0x0F);
        kprint("  shutdown - Mematikan sistem operasi dan hardware\n", 0x0F);
    } 
    else if (string_compare(command_buffer, "about")) {
        kprint("Lenovix OS v0.1\n", 0x0A);
        kprint("Sebuah proyek sistem operasi buatan anak bangsa.\n", 0x0B);
        kprint("Dikembangkan secara independen menggunakan IDT Interrupt.\n", 0x0B);
    } 
    else if (string_compare(command_buffer, "clear")) {
        clear_screen();
    } 
    else if (string_compare(command_buffer, "shutdown")) {
        kprint("Mematikan Lenovix OS...", 0x0C);
        for(volatile int i = 0; i < 50000000; i++); 
        shutdown_qemu();
    }
    else if (string_compare(command_buffer, "uptime")) {
        // Karena 100 ticks = 1 detik
        unsigned int total_seconds = timer_ticks / 100;
        char sec_str[16];
        int_to_string(total_seconds, sec_str);

        kprint("Lenovix OS telah aktif selama: ", 0x0F);
        kprint(sec_str, 0x0A);
        kprint(" detik.\n", 0x0F);
    }
    else {
        kprint("Perintah '", 0x0C);
        kprint(command_buffer, 0x0C);
        kprint("' tidak ditemukan. Ketik 'help' untuk bantuan.\n", 0x0C);
    }

    kprint("> ", 0x0F);
    command_index = 0;
}

unsigned char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};
int shift_pressed = 0;
int caps_lock_active = 0;
unsigned char scancode_to_ascii_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0,
  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0, '*',   0, ' '
};

// FUNGSI BARU: Dipanggil langsung oleh sistem interupsi Assembly
void keyboard_handler_c(void) {
    unsigned char scancode = inb(0x60); // Baca data tombol dari port keyboard

    // 1. Cek jika tombol Shift ditekan (Press)
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    }
    // 2. Cek jika tombol Shift dilepas (Release)
    else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    }
    // 3. Cek jika tombol Caps Lock ditekan (Toggle status)
    else if (scancode == 0x3A) {
        caps_lock_active = !caps_lock_active; // Balikkan status (jika 1 jadi 0, jika 0 jadi 1)
    }
    // 4. Proses tombol alfabet dan karakter biasa
    else if (scancode < 0x80) {
        unsigned char ascii = 0;
        
        // Cek apakah scancode yang ditekan termasuk huruf alfabet (A-Z / a-z)
        // Berdasarkan tabel, scancode huruf utama berkisar antara 0x10 (Q) sampai 0x32 (M)
        int is_letter = (scancode >= 0x10 && scancode <= 0x19) || // Baris Q-P
                        (scancode >= 0x1E && scancode <= 0x26) || // Baris A-L
                        (scancode >= 0x2C && scancode <= 0x32);   // Baris Z-M

        if (is_letter) {
            // Untuk HURUF: Caps Lock dan Shift saling meniadakan jika aktif bersamaan (XOR logic)
            if (caps_lock_active ^ shift_pressed) {
                ascii = scancode_to_ascii_shift[scancode];
            } else {
                ascii = scancode_to_ascii[scancode];
            }
        } else {
            // Untuk SIMBOL/ANGKA: Hanya terpengaruh oleh tombol Shift, Caps Lock diabaikan
            ascii = shift_pressed ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
        }
        
        // Cetak karakter ke layar shell
        if (ascii != 0) {
            if (ascii == '\n') {
                process_command();
            } else if (ascii == '\b') {
                if (command_index > 0) {
                    command_index--;
                    kprint_char(ascii, 0x0F);
                }
            } else {
                if (command_index < 255) {
                    command_buffer[command_index] = ascii;
                    command_index++;
                    kprint_char(ascii, 0x0F);
                }
            }
        }
    }

    // WAJIB: Kirim sinyal EOI (End of Interrupt) ke PIC agar bersedia menerima interupsi berikutnya
    outb(0x20, 0x20);
}

void kernel_main(struct multiboot_info* mb_info) { // Tambahkan parameter struct
    init_gdt();
    clear_screen();

    kprint("Selamat Datang di Lenovix OS!\n", 0x0A);
    kprint("-----------------------------\n", 0x07);
    kprint("Inisialisasi GDT Proteksi Memori... [ OK ]\n", 0x0A);
    
    kprint("Mengaktifkan IDT & Hardware Interrupts... ", 0x0F);
    init_idt();
    kprint("[ OK ]\n", 0x0A);
    
    init_timer();
    
    // Inisialisasi Physical Memory Manager
    kprint("Mengaktifkan Physical Memory Manager (PMM)... ", 0x0F);
    init_pmm(mb_info, (unsigned int)&end);
    kprint("[ OK ]\n", 0x0A);

    kprint("Silakan ketik perintah Anda:\n\n> ", 0x0E);

    while(1) {
        asm volatile("hlt");
    }
}