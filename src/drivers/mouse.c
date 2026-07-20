#include "mouse.h"
#include "vesa.h"

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static mouse_state_t mouse_state = {512, 384, 0, 0, 0};
static unsigned char mouse_cycle = 0;
static char mouse_packet[3];

// Bitmap Kursor Panah Sederhana (12x18 pixel)
static const unsigned char cursor_sprite[18][12] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0},
    {1,2,2,2,2,2,1,1,1,1,1,0},
    {1,2,2,1,2,2,1,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0},
    {1,1,0,0,1,2,2,1,0,0,0,0},
    {1,0,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
};

static void mouse_wait(unsigned char type) {
    unsigned int timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static void mouse_write(unsigned char write_val) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write_val);
}

static unsigned char mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init(void) {
    // 1. Enable Auxiliary Device (Mouse PS/2)
    mouse_wait(1);
    outb(0x64, 0xA8);

    // 2. Dapatkan Command Byte Kontroler
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    unsigned char status = inb(0x60);

    // Set bit 1 (enable mouse IRQ12) dan bit 0 (enable keyboard IRQ1)
    status |= 0x02;
    status &= ~0x20; // Clear disable mouse flag

    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    // 3. Reset ke Default Settings
    mouse_write(0xF6);
    mouse_read(); // ACK (0xFA)

    // 4. Enable Data Reporting
    mouse_write(0xF4);
    mouse_read(); // ACK (0xFA)
}

void mouse_handler(void) {
    // Periksa apakah ada data di buffer status port 0x64
    unsigned char status = inb(0x64);
    if (!(status & 0x01)) return; // Tidak ada data sama sekali

    unsigned char data = inb(0x60);

    // Cek apakah data berasal dari AUX (Mouse)
    if (!(status & 0x20) && mouse_cycle == 0) return;

    switch (mouse_cycle) {
        case 0:
            // Byte 0: Bit 3 harus selalu 1 untuk synchro paket PS/2
            if ((data & 0x08) == 0x08) {
                mouse_packet[0] = data;
                mouse_cycle++;
            }
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_packet[2] = data;
            mouse_cycle = 0;

            // Tombol Klik
            mouse_state.left_button   = mouse_packet[0] & 0x01;
            mouse_state.right_button  = (mouse_packet[0] >> 1) & 0x01;
            mouse_state.middle_button = (mouse_packet[0] >> 2) & 0x01;

            // Hitung Perubahan Posisi (Delta X dan Y) dengan penanganan Sign Extension
            int rel_x = (int)mouse_packet[1];
            int rel_y = (int)mouse_packet[2];

            if (mouse_packet[0] & 0x10) rel_x |= 0xFFFFFF00; // Sign extend X negative
            if (mouse_packet[0] & 0x20) rel_y |= 0xFFFFFF00; // Sign extend Y negative

            mouse_state.x += rel_x;
            mouse_state.y -= rel_y; // Invert Y sumbu layar

            // Clamp koordinat agar tetap di dalam layar 1024x768
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.x >= 1024) mouse_state.x = 1023;
            if (mouse_state.y >= 768) mouse_state.y = 767;
            break;
    }
}

mouse_state_t* mouse_get_state(void) {
    return &mouse_state;
}

void vesa_draw_cursor(int x, int y) {
    for (int row = 0; row < 18; row++) {
        for (int col = 0; col < 12; col++) {
            unsigned char pixel = cursor_sprite[row][col];
            if (pixel == 1) { // Border Hitam
                vesa_put_pixel(x + col, y + row, 0x000000);
            } else if (pixel == 2) { // Isi Putih
                vesa_put_pixel(x + col, y + row, 0xFFFFFF);
            }
        }
    }
}