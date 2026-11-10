struct gdt_entry_struct
{
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));

struct gdt_ptr_struct
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct gdt_entry_struct gdt[6];
struct gdt_ptr_struct gdt_ptr;

struct tss_entry_struct
{
    unsigned int prev_tss;
    unsigned int esp0;
    unsigned int ss0;
    unsigned int esp1;
    unsigned int ss1;
    unsigned int esp2;
    unsigned int ss2;
    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;
    unsigned int eax;
    unsigned int ecx;
    unsigned int edx;
    unsigned int ebx;
    unsigned int esp;
    unsigned int ebp;
    unsigned int esi;
    unsigned int edi;
    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
    unsigned int fs;
    unsigned int gs;
    unsigned int ldt;
    unsigned short trap;
    unsigned short iomap_base;
} __attribute__((packed));

struct tss_entry_struct tss_entry;

extern void gdt_flush(unsigned int gdt_ptr_address);
extern void tss_flush(void);

void set_gdt_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void write_tss(int num, unsigned short ss0, unsigned int esp0)
{
    unsigned int base = (unsigned int)&tss_entry;
    unsigned int limit = sizeof(tss_entry) - 1;

    set_gdt_gate(num, base, limit, 0xE9, 0x00);

    char *ptr = (char *)&tss_entry;
    for (unsigned int i = 0; i < sizeof(tss_entry); i++)
    {
        ptr[i] = 0;
    }

    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
    tss_entry.cs = 0x08 | 3;
    tss_entry.ss = 0x10 | 3;
    tss_entry.ds = 0x10 | 3;
    tss_entry.es = 0x10 | 3;
    tss_entry.fs = 0x10 | 3;
    tss_entry.gs = 0x10 | 3;
    tss_entry.iomap_base = sizeof(tss_entry);
}

void tss_set_stack(unsigned int stack)
{
    tss_entry.esp0 = stack;
}

void init_gdt(void)
{
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 6) - 1;
    gdt_ptr.base = (unsigned int)&gdt;
    set_gdt_gate(0, 0, 0, 0, 0);
    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    write_tss(5, 0x10, 0x0);
    gdt_flush((unsigned int)&gdt_ptr);
    tss_flush();
}