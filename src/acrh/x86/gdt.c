// Struktur untuk setiap baris di tabel GDT (GDT Entry)
struct gdt_entry_struct {
    unsigned short limit_low;   // 16-bit bawah dari batas memori
    unsigned short base_low;    // 16-bit bawah dari alamat awal memori
    unsigned char  base_middle; // 8-bit tengah dari alamat awal
    unsigned char  access;      // Hak akses (Ring level, Read/Write, Executable)
    unsigned char  granularity; // Atribut ukuran (4KB/1Byte blocks, 32-bit mode)
    unsigned char  base_high;   // 8-bit atas dari alamat awal
} __attribute__((packed));

// Struktur pointer GDT yang dibaca langsung oleh register CPU
struct gdt_ptr_struct {
    unsigned short limit;       // Ukuran tabel GDT dikurang 1
    unsigned int   base;        // Alamat awal tabel GDT di RAM
} __attribute__((packed));

// SESUDAH: Kita membuat 5 entri (0: Null, 1: Kernel Code, 2: Kernel Data, 3: User Code, 4: User Data)
struct gdt_entry_struct gdt[5];
struct gdt_ptr_struct gdt_ptr;

// Hubungkan fungsi Assembly dari boot.asm
extern void gdt_flush(unsigned int gdt_ptr_address);

// Fungsi untuk mengisi nilai per entri GDT
void set_gdt_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

// Inisialisasi awal GDT
void init_gdt(void) {
    // SEBELUM: gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 3) - 1;
    
    // SESUDAH: Sesuaikan dengan 5 entri
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 5) - 1;
    gdt_ptr.base  = (unsigned int)&gdt;

    // 1. Entry 0: Null Descriptor
    set_gdt_gate(0, 0, 0, 0, 0);

    // 2. Entry 1: Kernel Code Segment (0x08)
    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 3. Entry 2: Kernel Data Segment (0x10)
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // 4. Entry 3: User Mode Code Segment (0x1B)
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); 

    // 5. Entry 4: User Mode Data Segment (0x23)
    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // Muat ke CPU
    gdt_flush((unsigned int)&gdt_ptr);
}