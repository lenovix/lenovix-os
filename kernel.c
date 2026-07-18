int cursor_x = 0;
int cursor_y = 0;
const int VGA_WIDTH = 80;
const int VGA_HEIGHT = 25;
char *video_memory = (char *) 0xB8000;

// Buffer untuk menampung perintah yang sedang diketik (maksimal 256 karakter)
char command_buffer[256];
int command_index = 0;

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);
extern void shutdown_qemu(void);

void update_hardware_cursor(void) {
    // Hitung posisi linear di memori VGA (0 sampai 1999)
    unsigned short position = cursor_y * VGA_WIDTH + cursor_x;

    // Kirim perintah ke CRT Controller Register untuk mengatur posisi kursor
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));        // Byte bawah (low)
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF)); // Byte atas (high)
}

// Fungsi buatan sendiri untuk membandingkan dua string (seperti strcmp di C standar)
int string_compare(const char *str1, const char *str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return 0; // Tidak cocok
        }
        i++;
    }
    // Jika kedua string berakhir di saat yang sama, berarti cocok sempurna
    if (str1[i] == '\0' && str2[i] == '\0') {
        return 1; 
    }
    return 0;
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

void kprint_char(char c, char color) {
    if (c == '\b') {
        if (cursor_x > 2) { // Batasi agar tidak menghapus prompt "> " (panjangnya 2 karakter)
            cursor_x--;
            int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
            video_memory[index] = ' ';
            video_memory[index + 1] = color;
        }
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
    update_hardware_cursor();
}

void kprint(const char *str, char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        kprint_char(str[i], color);
    }
}

// Fungsi Shell Interpreter untuk memproses perintah setelah tombol Enter ditekan
void process_command(void) {
    kprint("\n", 0x07); // Pindah baris baru sebelum menampilkan output

    if (command_index == 0) {
        // Jika user hanya menekan Enter tanpa mengetik apa-apa
        kprint("> ", 0x0F);
        return;
    }

    // Akhiri string di buffer dengan null-terminator
    command_buffer[command_index] = '\0';

    // Pilihan Perintah 1: help
    if (string_compare(command_buffer, "help")) {
        kprint("Perintah Lenovix OS yang tersedia:\n", 0x0E);
        kprint("  help  - Menampilkan daftar perintah ini\n", 0x0F);
        kprint("  about - Informasi mengenai sistem operasi ini\n", 0x0F);
        kprint("  clear - Membersihkan layar shell\n", 0x0F);
        kprint("  shutdown - Mematikan sistem operasi\n", 0x0F);
    } 
    // Pilihan Perintah 2: about
    else if (string_compare(command_buffer, "about")) {
        kprint("Lenovix OS v0.1\n", 0x0A);
        kprint("Sebuah proyek sistem operasi buatan anak bangsa.\n", 0x0B);
        kprint("Dikembangkan secara independen langsung di atas hardware.\n", 0x0B);
    } 
    // Pilihan Perintah 3: clear
    else if (string_compare(command_buffer, "clear")) {
        clear_screen();
    } 
    // Pilihan Perintah 4: shutdown
    else if (string_compare(command_buffer, "shutdown")) {
        kprint("Mematikan Lenovix OS...", 0x0C); // Teks merah
        
        // Beri sedikit jeda visual sebelum mati (opsional, loop kosong)
        for(volatile int i = 0; i < 50000000; i++); 
        
        shutdown_qemu();
    }
    // Perintah tidak dikenali
    else {
        kprint("Perintah '", 0x0C);
        kprint(command_buffer, 0x0C);
        kprint("' tidak ditemukan. Ketik 'help' untuk bantuan.\n", 0x0C);
    }

    // Tampilkan prompt kembali (kecuali jika layar baru di-clear)
    if (!string_compare(command_buffer, "clear")) {
        kprint("> ", 0x0F);
    } else {
        kprint("> ", 0x0F);
    }

    // Reset buffer untuk perintah berikutnya
    command_index = 0;
}

unsigned char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

void wait_for_keyboard(void) {
    unsigned char last_scancode = 0;

    while(1) {
        if (inb(0x64) & 1) {
            unsigned char scancode = inb(0x60);

            if (scancode < 0x80 && scancode != last_scancode) {
                unsigned char ascii = scancode_to_ascii[scancode];
                
                if (ascii != 0) {
                    // Jika tombol Enter ditekan
                    if (ascii == '\n') {
                        process_command();
                    } 
                    // Jika tombol Backspace ditekan
                    else if (ascii == '\b') {
                        if (command_index > 0) {
                            command_index--;
                            kprint_char(ascii, 0x0F);
                        }
                    } 
                    // Jika karakter teks biasa dan kapasitas buffer belum penuh
                    else {
                        if (command_index < 255) {
                            command_buffer[command_index] = ascii;
                            command_index++;
                            kprint_char(ascii, 0x0F);
                        }
                    }
                }
                last_scancode = scancode;
            } 
            else if (scancode >= 0x80) {
                last_scancode = 0;
            }
        }
    }
}

void kernel_main(void) {
    clear_screen();

    kprint("Selamat Datang di Lenovix OS!\n", 0x0A);
    kprint("-----------------------------\n", 0x07);
    kprint("Ketik perintah Anda di bawah (contoh: help, about, clear).\n\n> ", 0x0E);

    wait_for_keyboard();
}