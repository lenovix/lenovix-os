#ifndef PCI_H
#define PCI_H

#define MAX_PCI_DEVICES 32

typedef struct
{
    unsigned char bus;
    unsigned char slot;
    unsigned char func;
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char class_id;
    unsigned char subclass_id;
    unsigned int bar0;
} pci_device_t;

void pci_init(void);
int pci_get_count(void);
pci_device_t *pci_get_device(int idx);
pci_device_t *pci_find_device(unsigned short vendor_id, unsigned short device_id);

unsigned int pci_read_config_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset);
unsigned short pci_read_config_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset);
void pci_write_config_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned short val);
void pci_write_config_dword(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int val);

#endif
