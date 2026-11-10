#include "tty.h"

struct idt_entry_struct
{
    unsigned short base_low;
    unsigned short sel;
    unsigned char always0;
    unsigned char flags;
    unsigned short base_high;
} __attribute__((packed));

struct idt_ptr_struct
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

typedef struct
{
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
} __attribute__((packed)) registers_t;

struct idt_entry_struct idt[256];
struct idt_ptr_struct idt_ptr;

extern void load_idt(unsigned int idt_ptr_address);
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);
extern void keyboard_handler_asm(void);
extern void mouse_handler_asm(void);
extern void default_handler_asm(void);
extern void timer_handler_asm(void);
extern void syscall_handler(void);

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

static const char *exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"};

static void serial_putc(char c)
{
    while ((inb(0x3FD) & 0x20) == 0)
    {
    }
    outb(0x3F8, c);
}

static void serial_puts(const char *str)
{
    while (*str)
    {
        if (*str == '\n')
            serial_putc('\r');
        serial_putc(*str++);
    }
}

static void print_hex(unsigned int val)
{
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--)
    {
        buf[2 + (7 - i)] = hex_chars[(val >> (i * 4)) & 0xF];
    }
    buf[10] = '\0';
    kprint(buf, 0x0C);
    serial_puts(buf);
}

void isr_exception_handler(registers_t *regs)
{
    kprint("\n[CPU EXCEPTION] ", 0x0C);
    serial_puts("\n[CPU EXCEPTION] ");
    if (regs->int_no < 32)
    {
        kprint(exception_messages[regs->int_no], 0x0C);
        serial_puts(exception_messages[regs->int_no]);
    }
    else
    {
        kprint("Unknown Exception", 0x0C);
        serial_puts("Unknown Exception");
    }
    kprint(" | Code: ", 0x0C);
    serial_puts(" | Code: ");
    print_hex(regs->err_code);
    kprint(" | EIP: ", 0x0C);
    serial_puts(" | EIP: ");
    print_hex(regs->eip);
    kprint("\n", 0x0C);
    serial_puts("\n");

    if (regs->int_no == 14)
    {
        unsigned int cr2;
        __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
        kprint("[Page Fault] Faulting Address: ", 0x0E);
        serial_puts("[Page Fault] Faulting Address: ");
        print_hex(cr2);
        kprint("\n", 0x0E);
        serial_puts("\n");
    }

    kprint("Kernel Panic: System Halted.\n", 0x4F);
    serial_puts("Kernel Panic: System Halted.\n");
    __asm__ __volatile__("cli; hlt");
    while (1)
    {
    }
}

void set_idt_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags)
{
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void default_handler_c(void)
{
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void remap_pic(void)
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void init_idt(void)
{
    idt_ptr.limit = (sizeof(struct idt_entry_struct) * 256) - 1;
    idt_ptr.base = (unsigned int)&idt;

    remap_pic();

    set_idt_gate(0, (unsigned int)isr0, 0x08, 0x8E);
    set_idt_gate(1, (unsigned int)isr1, 0x08, 0x8E);
    set_idt_gate(2, (unsigned int)isr2, 0x08, 0x8E);
    set_idt_gate(3, (unsigned int)isr3, 0x08, 0x8E);
    set_idt_gate(4, (unsigned int)isr4, 0x08, 0x8E);
    set_idt_gate(5, (unsigned int)isr5, 0x08, 0x8E);
    set_idt_gate(6, (unsigned int)isr6, 0x08, 0x8E);
    set_idt_gate(7, (unsigned int)isr7, 0x08, 0x8E);
    set_idt_gate(8, (unsigned int)isr8, 0x08, 0x8E);
    set_idt_gate(9, (unsigned int)isr9, 0x08, 0x8E);
    set_idt_gate(10, (unsigned int)isr10, 0x08, 0x8E);
    set_idt_gate(11, (unsigned int)isr11, 0x08, 0x8E);
    set_idt_gate(12, (unsigned int)isr12, 0x08, 0x8E);
    set_idt_gate(13, (unsigned int)isr13, 0x08, 0x8E);
    set_idt_gate(14, (unsigned int)isr14, 0x08, 0x8E);
    set_idt_gate(15, (unsigned int)isr15, 0x08, 0x8E);
    set_idt_gate(16, (unsigned int)isr16, 0x08, 0x8E);
    set_idt_gate(17, (unsigned int)isr17, 0x08, 0x8E);
    set_idt_gate(18, (unsigned int)isr18, 0x08, 0x8E);
    set_idt_gate(19, (unsigned int)isr19, 0x08, 0x8E);
    set_idt_gate(20, (unsigned int)isr20, 0x08, 0x8E);
    set_idt_gate(21, (unsigned int)isr21, 0x08, 0x8E);
    set_idt_gate(22, (unsigned int)isr22, 0x08, 0x8E);
    set_idt_gate(23, (unsigned int)isr23, 0x08, 0x8E);
    set_idt_gate(24, (unsigned int)isr24, 0x08, 0x8E);
    set_idt_gate(25, (unsigned int)isr25, 0x08, 0x8E);
    set_idt_gate(26, (unsigned int)isr26, 0x08, 0x8E);
    set_idt_gate(27, (unsigned int)isr27, 0x08, 0x8E);
    set_idt_gate(28, (unsigned int)isr28, 0x08, 0x8E);
    set_idt_gate(29, (unsigned int)isr29, 0x08, 0x8E);
    set_idt_gate(30, (unsigned int)isr30, 0x08, 0x8E);
    set_idt_gate(31, (unsigned int)isr31, 0x08, 0x8E);

    for (int i = 32; i < 256; i++)
    {
        set_idt_gate(i, (unsigned int)default_handler_asm, 0x08, 0x8E);
    }

    set_idt_gate(32, (unsigned int)timer_handler_asm, 0x08, 0x8E);
    set_idt_gate(33, (unsigned int)keyboard_handler_asm, 0x08, 0x8E);
    set_idt_gate(44, (unsigned int)mouse_handler_asm, 0x08, 0x8E);
    set_idt_gate(0x80, (unsigned int)syscall_handler, 0x08, 0xEE);
    load_idt((unsigned int)&idt_ptr);
}