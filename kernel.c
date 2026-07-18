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
    update_hardware_cursor();
}

void kprint(const char *str, char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        kprint_char(str[i], color);
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

// FUNGSI BARU: Dipanggil langsung oleh sistem interupsi Assembly
void keyboard_handler_c(void) {
    unsigned char scancode = inb(0x60); // Baca data tombol dari port keyboard

    // Sinyal di bawah 0x80 artinya tombol ditekan
    if (scancode < 0x80) {
        unsigned char ascii = scancode_to_ascii[scancode];
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

void kernel_main(void) {
    clear_screen();

    kprint("Selamat Datang di Lenovix OS!\n", 0x0A);
    kprint("-----------------------------\n", 0x07);
    
    // Aktifkan IDT & Interrupt System
    kprint("Mengaktifkan IDT & Hardware Interrupts... ", 0x0F);
    init_idt();
    kprint("[ OK ]\n", 0x0A);
    
    kprint("Silakan ketik perintah Anda:\n\n> ", 0x0E);

    // CPU sekarang bisa santai, tidak ada loop polling keyboard lagi!
    while(1) {
        asm volatile("hlt"); // Perintah hemat daya: CPU tidur sampai ada interupsi masuk
    }
}