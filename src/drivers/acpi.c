#include "acpi.h"
#include "io_util.h"

struct rsdp_descriptor
{
    char signature[8];
    unsigned char checksum;
    char oem_id[6];
    unsigned char revision;
    unsigned int rsdt_addr;
} __attribute__((packed));

struct acpi_header
{
    char signature[4];
    unsigned int length;
    unsigned char revision;
    unsigned char checksum;
    char oem_id[6];
    char oem_table_id[8];
    unsigned int oem_revision;
    unsigned int creator_id;
    unsigned int creator_revision;
} __attribute__((packed));

struct fadt_table
{
    struct acpi_header header;
    unsigned int firmware_ctrl;
    unsigned int dsdt_addr;
    unsigned char reserved;
    unsigned char pm_profile;
    unsigned short sci_int;
    unsigned int smi_cmd;
    unsigned char acpi_enable;
    unsigned char acpi_disable;
    unsigned char s4bios_req;
    unsigned char pstate_cnt;
    unsigned int pm1a_evt_blk;
    unsigned int pm1b_evt_blk;
    unsigned int pm1a_cnt_blk;
    unsigned int pm1b_cnt_blk;
} __attribute__((packed));

static unsigned int pm1a_cnt = 0;
static unsigned int pm1b_cnt = 0;
static unsigned short slp_typa = 0;
static unsigned short slp_typb = 0;
static int acpi_ready = 0;

static int mem_cmp4(const char *a, const char *b)
{
    return (a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]);
}

static struct rsdp_descriptor *find_rsdp(void)
{
    unsigned char *mem = (unsigned char *)0x000E0000;
    while ((unsigned int)mem < 0x00100000)
    {
        if (mem[0] == 'R' && mem[1] == 'S' && mem[2] == 'D' && mem[3] == ' ' &&
            mem[4] == 'P' && mem[5] == 'T' && mem[6] == 'R' && mem[7] == ' ')
        {
            unsigned char sum = 0;
            for (int i = 0; i < 20; i++)
            {
                sum += mem[i];
            }
            if (sum == 0)
            {
                return (struct rsdp_descriptor *)mem;
            }
        }
        mem += 16;
    }
    return 0;
}

static int parse_dsdt_s5(unsigned char *dsdt, unsigned int len)
{
    for (unsigned int i = 0; i < len - 8; i++)
    {
        if (dsdt[i] == '_' && dsdt[i + 1] == 'S' && dsdt[i + 2] == '5' && dsdt[i + 3] == '_')
        {
            unsigned int idx = i + 4;
            if (dsdt[idx] == 0x12)
            {
                idx += 3;
                if (dsdt[idx] == 0x0A)
                    idx++;
                slp_typa = dsdt[idx++] << 10;

                if (dsdt[idx] == 0x0A)
                    idx++;
                slp_typb = dsdt[idx] << 10;
                return 0;
            }
        }
    }
    slp_typa = (5 << 10);
    slp_typb = (5 << 10);
    return -1;
}

int acpi_init(void)
{
    struct rsdp_descriptor *rsdp = find_rsdp();
    if (!rsdp || rsdp->rsdt_addr == 0)
    {
        acpi_ready = 0;
        return -1;
    }

    struct acpi_header *rsdt = (struct acpi_header *)rsdp->rsdt_addr;
    if (!mem_cmp4(rsdt->signature, "RSDT"))
    {
        acpi_ready = 0;
        return -2;
    }

    unsigned int entries = (rsdt->length - sizeof(struct acpi_header)) / 4;
    unsigned int *entry_ptr = (unsigned int *)((char *)rsdt + sizeof(struct acpi_header));

    for (unsigned int i = 0; i < entries; i++)
    {
        struct acpi_header *hdr = (struct acpi_header *)entry_ptr[i];
        if (hdr && mem_cmp4(hdr->signature, "FACP"))
        {
            struct fadt_table *fadt = (struct fadt_table *)hdr;
            pm1a_cnt = fadt->pm1a_cnt_blk;
            pm1b_cnt = fadt->pm1b_cnt_blk;

            if (fadt->dsdt_addr)
            {
                struct acpi_header *dsdt_hdr = (struct acpi_header *)fadt->dsdt_addr;
                parse_dsdt_s5((unsigned char *)dsdt_hdr, dsdt_hdr->length);
            }
            acpi_ready = 1;
            return 0;
        }
    }

    acpi_ready = 0;
    return -3;
}

void acpi_power_off(void)
{
    if (acpi_ready && pm1a_cnt)
    {
        outw((unsigned short)pm1a_cnt, slp_typa | (1 << 13));
        if (pm1b_cnt)
        {
            outw((unsigned short)pm1b_cnt, slp_typb | (1 << 13));
        }
    }

    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);

    __asm__ __volatile__("cli; hlt");
    while (1)
    {
    }
}

void acpi_reboot(void)
{
    unsigned char status;
    int timeout = 100000;
    do
    {
        status = inb(0x64);
    } while ((status & 0x02) && --timeout);

    outb(0x64, 0xFE);

    struct
    {
        unsigned short limit;
        unsigned int base;
    } __attribute__((packed)) null_idt = {0, 0};

    __asm__ __volatile__("lidt %0; int3" : : "m"(null_idt));
    while (1)
    {
    }
}
