#include "task.h"

extern void outb(unsigned short port, unsigned char data);
extern void kprint(const char *str, char color);

// Variabel global untuk menampung jumlah detak yang terjadi
unsigned int timer_ticks = 0;

// Fungsi handler yang dipanggil oleh timer_handler_asm setiap 10ms
void timer_handler_c(void) {
    timer_ticks++;

    // Kirim EOI ke Master PIC agar bersedia menerima interupsi berikutnya
    outb(0x20, 0x20);
    schedule();
}

// Inisialisasi PIT ke frekuensi 100 Hz
void init_timer(void) {
    // Kebalikan dari frekuensi yang diinginkan (1193182 Hz / 100 Hz = 11931)
    unsigned int divisor = 1193182 / 100;

    // Kirim perintah ke Command Register PIT (Port 0x43)
    // 0x36 = Set Channel 0, Square Wave Mode, Akses Square/Low Byte lalu High Byte
    outb(0x43, 0x36);

    // Kirim nilai pembagi (divisor) ke Data Register Channel 0 (Port 0x40)
    outb(0x40, (unsigned char)(divisor & 0xFF));        // Low byte
    outb(0x40, (unsigned char)((divisor >> 8) & 0xFF)); // High byte
}

// Fungsi Delay/Sleep berbasis detak hardware (1 tick = 10ms)
void sleep(unsigned int ticks) {
    unsigned int start_ticks = timer_ticks;
    while (timer_ticks < start_ticks + ticks) {
        // CPU tidur hemat daya selama menunggu detak berikutnya
        asm volatile("hlt"); 
    }
}