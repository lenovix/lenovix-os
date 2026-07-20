struct idt_entry_struct {
    unsigned short base_low;
    unsigned short sel;
    unsigned char  always0;
    unsigned char  flags;
    unsigned short base_high;
} __attribute__((packed));

struct idt_ptr_struct {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

struct idt_entry_struct idt[256];
struct idt_ptr_struct idt_ptr;

extern void load_idt(unsigned int idt_ptr_address);
extern void outb(unsigned short port, unsigned char data);
extern void keyboard_handler_asm(void);
extern void default_handler_asm(void); // Ambil handler default
extern void timer_handler_asm(void);
extern void syscall_handler(void);

void set_idt_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

// Fungsi C untuk merespons interupsi tidak dikenal (seperti timer PIT)
void default_handler_c(void) {
    // Kirim EOI ke Master & Slave PIC agar mereka tahu interupsi sudah diabaikan dengan aman
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void remap_pic(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20); // Master PIC di 0x20 (32)
    outb(0xA1, 0x28); // Slave PIC di 0x28 (40)
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void init_idt(void) {
    idt_ptr.limit = (sizeof(struct idt_entry_struct) * 256) - 1;
    idt_ptr.base  = (unsigned int)&idt;

    remap_pic();

   // 1. Isi semua dengan default handler terlebih dahulu
    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, (unsigned int)default_handler_asm, 0x08, 0x8E);
    }

    // 2. Daftarkan IRQ 0 (Timer) ke nomor 32
    set_idt_gate(32, (unsigned int)timer_handler_asm, 0x08, 0x8E); // <-- TAMBAHKAN INI

    // 3. Daftarkan IRQ 1 (Keyboard) ke nomor 33
    set_idt_gate(33, (unsigned int)keyboard_handler_asm, 0x08, 0x8E);

    // Flag 0xEE membolehkan perintah 'int 0x80' dipanggil dari Ring 3
    set_idt_gate(0x80, (unsigned int)syscall_handler, 0x08, 0xEE);

    load_idt((unsigned int)&idt_ptr);
}