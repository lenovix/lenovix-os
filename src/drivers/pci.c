#include "pci.h"
#include "io_util.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

unsigned int pci_read_config_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset)
{
    unsigned int address = (1U << 31) |
                           ((unsigned int)bus << 16) |
                           ((unsigned int)slot << 11) |
                           ((unsigned int)func << 8) |
                           (offset & 0xFC);

    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

unsigned short pci_read_config_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset)
{
    unsigned int dword = pci_read_config_dword(bus, slot, func, offset);
    return (unsigned short)((dword >> ((offset & 2) * 8)) & 0xFFFF);
}

void pci_write_config_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int val)
{
    unsigned int address = (1U << 31) |
                           ((unsigned int)bus << 16) |
                           ((unsigned int)slot << 11) |
                           ((unsigned int)func << 8) |
                           (offset & 0xFC);

    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, val);
}

void pci_write_config_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned short val)
{
    unsigned int dword = pci_read_config_dword(bus, slot, func, offset);
    int shift = (offset & 2) * 8;
    dword &= ~(0xFFFFU << shift);
    dword |= ((unsigned int)val << shift);
    pci_write_config_dword(bus, slot, func, offset, dword);
}

static void check_pci_function(unsigned char bus, unsigned char slot, unsigned char func)
{
    unsigned short vendor_id = pci_read_config_word(bus, slot, func, 0x00);
    if (vendor_id == 0xFFFF || vendor_id == 0x0000)
    {
        return;
    }

    if (pci_device_count >= MAX_PCI_DEVICES)
    {
        return;
    }

    unsigned short device_id = pci_read_config_word(bus, slot, func, 0x02);
    unsigned short class_rev = pci_read_config_word(bus, slot, func, 0x0A);
    unsigned int bar0 = pci_read_config_dword(bus, slot, func, 0x10);

    pci_devices[pci_device_count].bus = bus;
    pci_devices[pci_device_count].slot = slot;
    pci_devices[pci_device_count].func = func;
    pci_devices[pci_device_count].vendor_id = vendor_id;
    pci_devices[pci_device_count].device_id = device_id;
    pci_devices[pci_device_count].subclass_id = class_rev & 0xFF;
    pci_devices[pci_device_count].class_id = (class_rev >> 8) & 0xFF;
    pci_devices[pci_device_count].bar0 = bar0;

    pci_device_count++;
}

static void check_pci_device(unsigned char bus, unsigned char slot)
{
    unsigned short vendor_id = pci_read_config_word(bus, slot, 0, 0x00);
    if (vendor_id == 0xFFFF || vendor_id == 0x0000)
    {
        return;
    }

    check_pci_function(bus, slot, 0);

    unsigned short header_type = pci_read_config_word(bus, slot, 0, 0x0E) & 0xFF;
    if (header_type & 0x80)
    {
        for (unsigned char func = 1; func < 8; func++)
        {
            if (pci_read_config_word(bus, slot, func, 0x00) != 0xFFFF)
            {
                check_pci_function(bus, slot, func);
            }
        }
    }
}

void pci_init(void)
{
    pci_device_count = 0;

    for (unsigned short bus = 0; bus < 8; bus++)
    {
        for (unsigned char slot = 0; slot < 32; slot++)
        {
            check_pci_device((unsigned char)bus, slot);
        }
    }
}

int pci_get_count(void)
{
    return pci_device_count;
}

pci_device_t *pci_get_device(int idx)
{
    if (idx >= 0 && idx < pci_device_count)
    {
        return &pci_devices[idx];
    }
    return 0;
}

pci_device_t *pci_find_device(unsigned short vendor_id, unsigned short device_id)
{
    for (int i = 0; i < pci_device_count; i++)
    {
        if (pci_devices[i].vendor_id == vendor_id && pci_devices[i].device_id == device_id)
        {
            return &pci_devices[i];
        }
    }
    return 0;
}
